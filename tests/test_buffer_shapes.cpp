#include "pjrt/buffer.hpp" // Include for pjrt::Buffer
#include "pjrt/client.hpp"
#include "pjrt/context.hpp"

#include <numeric> // For std::iota if needed
#include <string>  // For HLO string
#include <vector>

#include "gtest/gtest.h"

namespace {

// Test fixture for PJRT context and client
class BufferShapeTest : public ::testing::Test {
protected:
    pjrt::Context context_;
    pjrt::Client client_{context_};
    std::optional<pjrt::DeviceView> device_;

    void SetUp() override {
        // Get the first device for testing
        // Ensure client has devices available
        ASSERT_NO_THROW(device_ = client_.getFirstDevice());
        ASSERT_NE(device_->device_, nullptr) << "Failed to get a device for testing.";
    }
};

TEST_F(BufferShapeTest, TransferToDeviceShape) {
    // Test with a 2D shape
    std::vector<float> data_2d(6); // 2x3
    std::vector<int64_t> shape_2d = {2, 3};
    pjrt::Buffer buffer_2d = client_.transferToDevice(data_2d.data(), shape_2d, *device_).get();
    EXPECT_EQ(buffer_2d.dimensions(), shape_2d);

    // Test with a scalar shape (empty dimensions)
    float data_scalar = 1.0f;
    std::vector<int64_t> shape_scalar = {}; // Empty shape for scalar
    pjrt::Buffer buffer_scalar = client_.transferToDevice(&data_scalar, shape_scalar, *device_).get();
    EXPECT_EQ(buffer_scalar.dimensions(), shape_scalar);
    // Also test if PJRT C API might have given it {1} for a scalar passed with empty dims
    // This depends on PJRT_Client_BufferFromHostBuffer behavior when num_dims is 0.
    // The current C++ wrapper passes num_dims=0 for empty shape. Let's assume it results in rank 0.

    // Test with a 1D shape
    std::vector<float> data_1d(4);
    std::vector<int64_t> shape_1d = {4};
    pjrt::Buffer buffer_1d = client_.transferToDevice(data_1d.data(), shape_1d, *device_).get();
    EXPECT_EQ(buffer_1d.dimensions(), shape_1d);
}

TEST_F(BufferShapeTest, ExecuteScalarOutputShape) {
    const std::string scalar_hlo_str = R"delim(
module @jit_my_function attributes {mhlo.num_partitions = 1 : i32, mhlo.num_replicas = 1 : i32} {
  func.func public @main(%arg0: tensor<128xf32>) -> (tensor<128xf32> {jax.result_info = "result"}) {
    %cst = stablehlo.constant dense<1.000000e+00> : tensor<f32>
    %0 = stablehlo.broadcast_in_dim %cst, dims = [] : (tensor<f32>) -> tensor<128xf32>
    %1 = stablehlo.add %arg0, %0 : tensor<128xf32>
    return %1 : tensor<128xf32>
  }
})delim";
    // NOTE: There is a crazy issue in XLA's StableHlo parsing. If there is a newline at the end of the stable hlo string, compilation fails. If there is a newline followed by some whitespace, that does not fail.

    std::optional<pjrt::LoadedExecutable> executable;
    ASSERT_NO_THROW(executable = client_.compileFromStableHloString(scalar_hlo_str))
        << "Failed to compile scalar HLO program.";

    constexpr int kNumElements = 128;
    const std::vector<float> input_data(kNumElements, 1.0f); // e.g., f32[kNumElements]
    const std::vector<int64_t> input_shape = {kNumElements};
    pjrt::Buffer input_buffer = client_.transferToDevice(input_data.data(), input_shape, *device_).get();

    pjrt::Buffer output_buffer = executable->execute(*device_, input_buffer).get();

    // The expected output shape is the same as the input shape.
    EXPECT_EQ(output_buffer.dimensions(), input_shape)
        << "Output shape mismatch.";
}

// TODO: Add TEST_F(BufferShapeTest, ExecuteVectorOutputShape) if feasible

} // namespace
