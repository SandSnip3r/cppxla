//=======================================================================
// Copyright (c) 2017, Edgar E. Iglesias.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//=======================================================================

#ifndef MNIST_READER_HPP
#define MNIST_READER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace mnist {

/*!
 * \brief Represents a complete MNIST dataset
 * \tparam Pixel The type of a pixel
 * \tparam Label The type of a label
 */
template <template <typename...> class Container, typename Pixel, typename Label>
struct MNIST_dataset {
  Container<Container<Pixel>> training_images; ///< The training images
  Container<Label> training_labels;            ///< The training labels
  Container<Container<Pixel>> test_images;     ///< The test images
  Container<Label> test_labels;                ///< The test labels

  /*!
   * \brief Resize the training set to new_size
   *
   * If new_size is smaller than the current size, this function has no effect.
   *
   * \param new_size The new size of the training set
   */
  void resize_training(std::size_t new_size) {
    if (training_images.size() > new_size) {
      training_images.resize(new_size);
      training_labels.resize(new_size);
    }
  }

  /*!
   * \brief Resize the test set to new_size
   *
   * If new_size is smaller than the current size, this function has no effect.
   *
   * \param new_size The new size of the test set
   */
  void resize_test(std::size_t new_size) {
    if (test_images.size() > new_size) {
      test_images.resize(new_size);
      test_labels.resize(new_size);
    }
  }
};

/*!
 * \brief Read an unsigned 32-bit integer from a file stream
 * \param is The file stream
 * \return The unsigned 32-bit integer
 */
inline uint32_t read_u32(std::ifstream& is) {
  uint32_t n;
  is.read(reinterpret_cast<char*>(&n), 4);
  // TODO This is not portable, it should be replaced by a portable implementation
  n = __builtin_bswap32(n);
  return n;
}

/*!
 * \brief Read a collection of images from a file stream
 * \param is The file stream
 * \param n_images The number of images to read
 * \param n_rows The number of rows of the images
 * \param n_cols The number of columns of the images
 * \return A collection of images
 */
template <template <typename...> class Container, typename Pixel>
Container<Container<Pixel>> read_images(std::ifstream& is, uint32_t n_images, uint32_t n_rows, uint32_t n_cols) {
  Container<Container<Pixel>> images;
  images.resize(n_images);
  for (auto& image : images) {
    image.resize(n_rows * n_cols);
    is.read(reinterpret_cast<char*>(image.data()), n_rows * n_cols);
  }
  return images;
}

/*!
 * \brief Read a collection of labels from a file stream
 * \param is The file stream
 * \param n_labels The number of labels to read
 * \return A collection of labels
 */
template <template <typename...> class Container, typename Label>
Container<Label> read_labels(std::ifstream& is, uint32_t n_labels) {
  Container<Label> labels;
  labels.resize(n_labels);
  is.read(reinterpret_cast<char*>(labels.data()), n_labels);
  return labels;
}

/*!
 * \brief Read a MNIST dataset
 *
.
 * The dataset is assumed to be in the working directory
 *
 * \return The dataset
 */
template <template <typename...> class Container = std::vector, template <typename...> class SubContainer = std::vector, typename Pixel = uint8_t, typename Label = uint8_t>
MNIST_dataset<SubContainer, Pixel, Label> read_dataset(const std::string& folder = ".") {
  MNIST_dataset<SubContainer, Pixel, Label> dataset;

  // Read training images
  {
    std::ifstream is(folder + "/train-images-idx3-ubyte", std::ios::binary);
    if (is) {
      uint32_t magic_number = read_u32(is);
      if (magic_number != 0x803) {
        std::cout << "Invalid magic number for training images" << std::endl;
        return dataset;
      }
      uint32_t n_images = read_u32(is);
      uint32_t n_rows = read_u32(is);
      uint32_t n_cols = read_u32(is);
      dataset.training_images = read_images<SubContainer, Pixel>(is, n_images, n_rows, n_cols);
    } else {
      std::cout << "Failed to open training images file" << std::endl;
    }
  }

  // Read training labels
  {
    std::ifstream is(folder + "/train-labels-idx1-ubyte", std::ios::binary);
    if (is) {
      uint32_t magic_number = read_u32(is);
      if (magic_number != 0x801) {
        std::cout << "Invalid magic number for training labels" << std::endl;
        return dataset;
      }
      uint32_t n_labels = read_u32(is);
      dataset.training_labels = read_labels<SubContainer, Label>(is, n_labels);
    } else {
      std::cout << "Failed to open training labels file" << std::endl;
    }
  }

  // Read test images
  {
    std::ifstream is(folder + "/t10k-images-idx3-ubyte", std::ios::binary);
    if (is) {
      uint32_t magic_number = read_u32(is);
      if (magic_number != 0x803) {
        std::cout << "Invalid magic number for test images" << std::endl;
        return dataset;
      }
      uint32_t n_images = read_u32(is);
      uint32_t n_rows = read_u32(is);
      uint32_t n_cols = read_u32(is);
      dataset.test_images = read_images<SubContainer, Pixel>(is, n_images, n_rows, n_cols);
    } else {
      std::cout << "Failed to open test images file" << std::endl;
    }
  }

  // Read test labels
  {
    std::ifstream is(folder + "/t10k-labels-idx1-ubyte", std::ios::binary);
    if (is) {
      uint32_t magic_number = read_u32(is);
      if (magic_number != 0x801) {
        std::cout << "Invalid magic number for test labels" << std::endl;
        return dataset;
      }
      uint32_t n_labels = read_u32(is);
      dataset.test_labels = read_labels<SubContainer, Label>(is, n_labels);
    } else {
      std::cout << "Failed to open test labels file" << std::endl;
    }
  }

  return dataset;
}

} // end of namespace mnist

#endif