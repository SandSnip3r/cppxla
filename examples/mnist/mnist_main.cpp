#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include "mnist_reader.hpp"
#include "pjrt/client.hpp"

// Helper function to read a file into a string
std::string ReadFile(const std::string& file_path) {
  std::ifstream file(file_path);
  if (!file) {
    std::cerr << "Could not open file: " << file_path << std::endl;
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

int main() {
  try {
    // Initialize PJRT Client
    pjrt::Context context;
    pjrt::Client client(context);

    // Get PJRT Device
    pjrt::DeviceView device = client.getFirstDevice();

    // Load and Compile StableHLO Programs
    const std::string kInitModelHloFilename = "init_model.stablehlo";
    const std::string kInitOptimizerHloFilename = "init_optimizer.stablehlo";
    const std::string kTrainStepHloFilename = "train_step.stablehlo";
    const std::string init_model_hlo = ReadFile(kInitModelHloFilename);
    const std::string init_optimizer_hlo = ReadFile(kInitOptimizerHloFilename);
    const std::string train_step_hlo = ReadFile(kTrainStepHloFilename);

    if (init_model_hlo.empty() || init_optimizer_hlo.empty() || train_step_hlo.empty()) {
      std::cerr << "Failed to read one or more HLO files." << std::endl;
      return 1;
    }
    std::cout << "Successfully loaded stablehlo programs" << std::endl;

    std::cout << "Parsing & compiling \"" << kInitModelHloFilename << "\"" << std::endl;
    {
      std::ofstream tmpFile("tmp.txt");
      tmpFile << init_model_hlo;
    }
    pjrt::LoadedExecutable init_model_executable = client.compileFromStableHloString(init_model_hlo);
    std::cout << "Successfully compiled \"" << kInitModelHloFilename << "\"" << std::endl;

    std::cout << "Parsing & compiling \"" << kInitOptimizerHloFilename << "\"" << std::endl;
    {
      std::ofstream tmpFile("tmp.txt");
      tmpFile << init_optimizer_hlo;
    }
    pjrt::LoadedExecutable init_optimizer_executable = client.compileFromStableHloString(init_optimizer_hlo);
    std::cout << "Successfully compiled \"" << kInitOptimizerHloFilename << "\"" << std::endl;

    std::cout << "Parsing & compiling \"" << kTrainStepHloFilename << "\"" << std::endl;
    {
      std::ofstream tmpFile("tmp.txt");
      tmpFile << train_step_hlo;
    }
    pjrt::LoadedExecutable train_step_executable = client.compileFromStableHloString(train_step_hlo);
    std::cout << "Successfully compiled \"" << kTrainStepHloFilename << "\"" << std::endl;

    std::cout << "Successfully initialized PJRT client and compiled HLO programs." << std::endl;

    // Load MNIST Dataset
    mnist::MNIST_dataset<std::vector, uint8_t, uint8_t> dataset =
        mnist::read_dataset<std::vector, std::vector, uint8_t, uint8_t>(".");

    if (dataset.training_images.empty() || dataset.training_labels.empty()) {
      std::cerr << "Could not load MNIST training dataset." << std::endl;
      return 1;
    }
    std::cout << "Successfully loaded MNIST dataset." << std::endl;

    // Initialize Model and Optimizer State
    std::cout << "Copying model initializing seed to device" << std::endl;
    int32_t seed = 0;
    auto seed_buffer_future = client.transferToDevice(&seed, {}, device);
    pjrt::Buffer seed_buffer = seed_buffer_future.get();
    std::cout << "Successfully copied model initializing seed to device" << std::endl;

    std::cout << "Initializing model" << std::endl;
    std::vector<pjrt::Buffer*> init_model_args = {&seed_buffer};
    auto init_model_result = init_model_executable.execute(device, init_model_args);
    std::vector<pjrt::Buffer> model_params = init_model_result.get();
    std::cout << "Model initialized, got back " << model_params.size() << " buffers" << std::endl;

    std::cout << "Initializing optimizer" << std::endl;
    std::vector<pjrt::Buffer*> init_optimizer_args;
    auto init_optimizer_result = init_optimizer_executable.execute(device, init_optimizer_args);
    std::vector<pjrt::Buffer> optimizer_state = init_optimizer_result.get();
    std::cout << "Optimizer initialized, got back " << optimizer_state.size() << " buffers" << std::endl;

    std::cout << "Successfully initialized model and optimizer." << std::endl;

    // Training Loop
    const int num_steps = 1024;
    const int batch_size = 128;

    for (int step = 0; step < num_steps; ++step) {
      // Prepare batch
      std::vector<float> image_batch(batch_size * 28 * 28);
      std::vector<int32_t> label_batch(batch_size);
      for (int i = 0; i < batch_size; ++i) {
        int image_index = (step * batch_size + i) % dataset.training_images.size();
        for (int j = 0; j < 28 * 28; ++j) {
          image_batch[i * 28 * 28 + j] = static_cast<float>(dataset.training_images[image_index][j]) / 255.0f;
        }
        label_batch[i] = static_cast<int32_t>(dataset.training_labels[image_index]);
      }

      // Transfer data to device
      std::future<pjrt::Buffer> image_buffer_future = client.transferToDevice(image_batch.data(), {batch_size, 28, 28, 1}, device);
      std::future<pjrt::Buffer> label_buffer_future = client.transferToDevice(label_batch.data(), {batch_size}, device);

      pjrt::Buffer image_buffer = image_buffer_future.get();
      pjrt::Buffer label_buffer = label_buffer_future.get();

      // Execute training step
      std::vector<pjrt::Buffer*> argument_buffers;
      argument_buffers.reserve(model_params.size() + optimizer_state.size() + 2);
      for (pjrt::Buffer &buffer : model_params) {
        argument_buffers.push_back(&buffer);
      }
      for (pjrt::Buffer &buffer : optimizer_state) {
        argument_buffers.push_back(&buffer);
      }
      argument_buffers.push_back(&image_buffer);
      argument_buffers.push_back(&label_buffer);
      auto startTime = std::chrono::high_resolution_clock::now();
      std::future<std::vector<pjrt::Buffer>> train_step_future =
          train_step_executable.execute(device, argument_buffers);

      // Update model and optimizer state
      std::vector<pjrt::Buffer> train_step_result = train_step_future.get();
      int param_buffers_end_idx = model_params.size();
      int state_buffers_end_idx = param_buffers_end_idx + optimizer_state.size();
      for (int i = 0; i < param_buffers_end_idx; ++i) {
        model_params[i] = std::move(train_step_result[i]);
      }
      for (int i = param_buffers_end_idx; i < state_buffers_end_idx; ++i) {
        optimizer_state[i - param_buffers_end_idx] = std::move(train_step_result[i]);
      }
      pjrt::Buffer loss_buffer = std::move(train_step_result[state_buffers_end_idx]);

      // Report loss
      std::future<std::vector<float>> loss_future = loss_buffer.toHost<float>();
      std::vector<float> loss_vec = loss_future.get();
      float loss = loss_vec[0];
      auto endTime = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
      std::cout << "Step " << step << ": Loss = " << loss << " (" << duration << " us)" << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}