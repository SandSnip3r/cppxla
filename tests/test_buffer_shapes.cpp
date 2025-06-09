#include "pjrt/buffer.hpp" // Include for pjrt::Buffer
#include "pjrt/client.hpp"
#include "pjrt/context.hpp"

#include <fstream> // For file reading
#include <numeric> // For std::iota if needed
#include <sstream> // For file reading
#include <string>  // For HLO string
#include <vector>

#include "gtest/gtest.h"

// Helper function to read a file into a string (can be moved to a common test utility if many tests need it)
static std::string readFileIntoString(const std::string& path) {
    std::ifstream input_file(path);
    if (!input_file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    return buffer.str();
}

namespace {

// Test fixture for PJRT context and client
class BufferShapeTest : public ::testing::Test {
protected:
    pjrt::Context context_;
    pjrt::Client client_{context_};
    pjrt::DeviceView device_;

    void SetUp() override {
        // Get the first device for testing
        // Ensure client has devices available
        ASSERT_NO_THROW(device_ = client_.getFirstDevice());
        ASSERT_NE(device_.device_, nullptr) << "Failed to get a device for testing.";
    }
};

TEST_F(BufferShapeTest, TransferToDeviceShape) {
    // Test with a 2D shape
    std::vector<float> data_2d(6); // 2x3
    std::vector<int64_t> shape_2d = {2, 3};
    pjrt::Buffer buffer_2d = client_.transferToDevice(data_2d.data(), shape_2d, device_).get();
    EXPECT_EQ(buffer_2d.dimensions(), shape_2d);

    // Test with a scalar shape (empty dimensions)
    float data_scalar = 1.0f;
    std::vector<int64_t> shape_scalar = {}; // Empty shape for scalar
    pjrt::Buffer buffer_scalar = client_.transferToDevice(&data_scalar, shape_scalar, device_).get();
    EXPECT_EQ(buffer_scalar.dimensions(), shape_scalar);
    // Also test if PJRT C API might have given it {1} for a scalar passed with empty dims
    // This depends on PJRT_Client_BufferFromHostBuffer behavior when num_dims is 0.
    // The current C++ wrapper passes num_dims=0 for empty shape. Let's assume it results in rank 0.

    // Test with a 1D shape
    std::vector<float> data_1d(4);
    std::vector<int64_t> shape_1d = {4};
    pjrt::Buffer buffer_1d = client_.transferToDevice(data_1d.data(), shape_1d, device_).get();
    EXPECT_EQ(buffer_1d.dimensions(), shape_1d);
}

TEST_F(BufferShapeTest, ExecuteScalarOutputShape) {
    // HLO program for vector reduction (sum) resulting in a scalar
    // Reusing the HLO from examples/vector_add_1/myStableHlo.txt
    std::string scalar_hlo_program_path = "examples/vector_add_1/myStableHlo.txt";
    std::string scalar_hlo_str;
    ASSERT_NO_THROW(scalar_hlo_str = readFileIntoString(scalar_hlo_program_path))
        << "Failed to read HLO program: " << scalar_hlo_program_path;

    pjrt::LoadedExecutable executable;
    ASSERT_NO_THROW(executable = client_.compileFromStableHloString(scalar_hlo_str))
        << "Failed to compile scalar HLO program.";

    std::vector<float> input_data(10, 1.0f); // e.g., f32[10]
    std::vector<int64_t> input_shape = {10};
    pjrt::Buffer input_buffer = client_.transferToDevice(input_data.data(), input_shape, device_).get();

    pjrt::Buffer output_buffer = executable.execute(device_, input_buffer).get();

    // Expected shape for a scalar output is an empty vector (rank 0)
    std::vector<int64_t> expected_scalar_shape = {};
    // Or, if the C API consistently returns {1} for scalars from execute:
    // std::vector<int64_t> expected_scalar_shape = {1};
    // We need to confirm this behavior. PJRT_Buffer_Dimensions for a C API scalar buffer
    // typically yields num_dims = 0.
    EXPECT_EQ(output_buffer.dimensions(), expected_scalar_shape)
        << "Scalar output shape mismatch. Expected rank 0.";
}

// TODO: Add TEST_F(BufferShapeTest, ExecuteVectorOutputShape) if feasible

} // namespace
