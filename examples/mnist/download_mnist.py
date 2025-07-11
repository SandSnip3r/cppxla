
import torchvision
import os

# Create a directory to store the data
if not os.path.exists('mnist_data'):
    os.makedirs('mnist_data')

# Download the training and test sets
train_set = torchvision.datasets.MNIST(root='./mnist_data', train=True, download=True)
test_set = torchvision.datasets.MNIST(root='./mnist_data', train=False, download=True)

# The data is downloaded to mnist_data/MNIST/raw
# I will move the uncompressed files to the current directory and delete the compressed ones.
import shutil
raw_dir = 'mnist_data/MNIST/raw'
for f in os.listdir(raw_dir):
    if f.endswith('.gz'):
        os.remove(os.path.join(raw_dir, f))
    else:
        shutil.move(os.path.join(raw_dir, f), f)

# Clean up the empty directories
os.rmdir('mnist_data/MNIST/raw')
os.rmdir('mnist_data/MNIST')
os.rmdir('mnist_data')

print("MNIST dataset downloaded and moved to the current directory.")
