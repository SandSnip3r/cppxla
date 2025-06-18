import jax
import jax.numpy as jnp
from jax import grad, jit, random
import optax

# 1. Model Definition

def init_params(key, image_shape, num_classes):
    """Initializes the CNN parameters."""
    key, c1_key, b1_key, c2_key, b2_key, d1_key, b3_key, d2_key, b4_key = random.split(key, 9)

    # Convolutional Layer 1
    conv1_w = random.normal(c1_key, (3, 3, 1, 32))
    conv1_b = jnp.zeros((32,))

    # Convolutional Layer 2
    conv2_w = random.normal(c2_key, (3, 3, 32, 64))
    conv2_b = jnp.zeros((64,))

    # Dense Layer
    dense1_w = random.normal(d1_key, (7 * 7 * 64, 128))
    dense1_b = jnp.zeros((128,))

    # Output Layer
    dense2_w = random.normal(d2_key, (128, num_classes))
    dense2_b = jnp.zeros((num_classes,))

    return (conv1_w, conv1_b, conv2_w, conv2_b, dense1_w, dense1_b, dense2_w, dense2_b)

def apply_model(params, images):
    """Forward pass of the CNN."""
    conv1_w, conv1_b, conv2_w, conv2_b, dense1_w, dense1_b, dense2_w, dense2_b = params

    # Conv Stage 1
    x = jax.lax.conv_general_dilated(images, conv1_w, (1, 1), 'SAME', dimension_numbers=('NHWC', 'HWIO', 'NHWC')) + conv1_b
    x = jax.nn.relu(x)
    x = jax.lax.reduce_window(x, -jnp.inf, jax.lax.max, (1, 2, 2, 1), (1, 2, 2, 1), 'VALID')

    # Conv Stage 2
    x = jax.lax.conv_general_dilated(x, conv2_w, (1, 1), 'SAME', dimension_numbers=('NHWC', 'HWIO', 'NHWC')) + conv2_b
    x = jax.nn.relu(x)
    x = jax.lax.reduce_window(x, -jnp.inf, jax.lax.max, (1, 2, 2, 1), (1, 2, 2, 1), 'VALID')

    # Flatten
    x = jnp.reshape(x, (x.shape[0], -1))

    # Dense Layer
    x = jnp.dot(x, dense1_w) + dense1_b
    x = jax.nn.relu(x)

    # Output Layer
    x = jnp.dot(x, dense2_w) + dense2_b
    return x

# 2. Optimizer Setup

def create_optimizer(learning_rate=1e-3):
    """Creates an Adam optimizer."""
    return optax.adam(learning_rate)

# 3. Training Step Logic

def loss_fn(params, images, labels):
    """Calculates the cross-entropy loss."""
    logits = apply_model(params, images)
    one_hot_labels = jax.nn.one_hot(labels, num_classes=10)
    loss = optax.softmax_cross_entropy(logits, one_hot_labels).mean()
    return loss

def train_step(params, opt_state, optimizer, images, labels):
    """Performs a single training step."""
    loss, grads = jax.value_and_grad(loss_fn)(params, images, labels)
    updates, new_opt_state = optimizer.update(grads, opt_state)
    new_params = optax.apply_updates(params, updates)
    return new_params, new_opt_state, loss

# We JIT-compile train_step and mark the optimizer as a static argument.
# This is done outside the function definition.
jitted_train_step = jit(train_step, static_argnames=('optimizer',))

if __name__ == '__main__':
    # This section demonstrates how to get the StableHLO representation.
    # We will use this later to export the programs for the C++ API.

    # Dummy data for demonstration
    key = random.PRNGKey(0)
    dummy_images = jnp.ones((1, 28, 28, 1))
    dummy_labels = jnp.ones((1,), dtype=jnp.int32)
    num_classes = 10
    image_shape = dummy_images.shape

    # 1. Get StableHLO for model initialization
    def init_model_func(key):
        return init_params(key, image_shape, num_classes)

    lowered_init_model = jit(init_model_func).lower(key)
    stable_hlo_init_model = lowered_init_model.compiler_ir('stablehlo')
    with open("init_model.stablehlo", "w") as f:
        f.write(str(stable_hlo_init_model))
    print("Saved StableHLO for init_model to init_model.stablehlo")

    # 2. Get StableHLO for optimizer initialization
    # We need some initial params to initialize the optimizer state.
    params = init_params(key, image_shape, num_classes)
    optimizer = create_optimizer()
    
    def init_optimizer_func(params):
        return optimizer.init(params)

    lowered_init_optimizer = jit(init_optimizer_func).lower(params)
    stable_hlo_init_optimizer = lowered_init_optimizer.compiler_ir('stablehlo')
    with open("init_optimizer.stablehlo", "w") as f:
        f.write(str(stable_hlo_init_optimizer))
    print("Saved StableHLO for init_optimizer to init_optimizer.stablehlo")


    # 3. Get StableHLO for the training step
    opt_state = optimizer.init(params)
    lowered_train_step = jitted_train_step.lower(
        params, opt_state, optimizer, dummy_images, dummy_labels
    )
    stable_hlo_train_step = lowered_train_step.compiler_ir('stablehlo')
    with open("train_step.stablehlo", "w") as f:
        f.write(str(stable_hlo_train_step))
    print("Saved StableHLO for train_step to train_step.stablehlo")
