import jax
import jax.numpy as jnp

def main():
  # 1. Define a simple JAX function with some computation
  def my_function(x):
    return x+1

  # 2. JIT-compile the function
  jitted_function = jax.jit(my_function)

  # 3. Define example input for the function.
  #    This are needed to trace the function and generate the HLO.
  #    The shapes and dtypes of the input(s) will determine the specialization.
  example_x = jnp.zeros((128,), dtype=jnp.float32)

  # 4. Lower the JITted function to get the HLO computation.
  #    The .lower() method takes the example input.
  lowered_computation = jitted_function.lower(example_x)

  # 5. Get the StableHLO representation from the lowered computation.
  #    The compiler_ir() method can take a dialect argument.
  stablehlo_ir_module = lowered_computation.compiler_ir(dialect='stablehlo')

  # 6. Print the StableHLO to stdout.
  #    The result from compiler_ir() is an MLIR Module object,
  #    which can be converted to a string.
  if stablehlo_ir_module:
    print(str(stablehlo_ir_module))
  else:
    print("Could not retrieve StableHLO representation.")

if __name__ == '__main__':
  main()