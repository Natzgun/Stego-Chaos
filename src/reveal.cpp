
#include <bitset>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;
using namespace cv;

int rows, columns;
double x0 = 0.3;
double r = 3.9;
double a = 1.5;

vector<int> generateLogisticMapSequence(int total_pixels) {
  vector<int> indices(total_pixels);
  for (int i = 0; i < total_pixels; ++i) {
    indices[i] = i;
  }

  double x = x0;
  for (int i = 0; i < total_pixels; ++i) {
    x = r * x * (1.0 - x);
    int j = static_cast<int>(x * total_pixels) % total_pixels;
    swap(indices[i], indices[j]);
  }

  return indices;
}

vector<int> generateBernoulliSequence(int total_pixels) {
  vector<int> indices(total_pixels);

  for (int i = 0; i < total_pixels; ++i) {
    indices[i] = i;
  }

  double x = x0;
  for (int i = 0; i < total_pixels; ++i) {
    x = fmod(a * x, 1.0);
    int j = static_cast<int>(x * total_pixels) % total_pixels;
    swap(indices[i], indices[j]);
  }

  return indices;
}

Mat decryptImage(const Mat &encrypted_img) {
  int total_pixels = rows * columns;
  Mat img_decrypted(rows, columns, CV_8UC3);
  vector<int> perm_seq = generateBernoulliSequence(total_pixels);

  for (int i = 0; i < total_pixels; ++i) {
    int permuted_index = perm_seq[i];
    int x = i / columns;
    int y = i % columns;
    int px = permuted_index / columns;
    int py = permuted_index % columns;
    img_decrypted.at<Vec3b>(x, y) = encrypted_img.at<Vec3b>(px, py);
  }
  return img_decrypted;
}

Mat revealImage(Mat host) {
  Vec3b metadata_pixel_1 = host.at<Vec3b>(0, 0);
  Vec3b metadata_pixel_2 = host.at<Vec3b>(0, 1);

  rows = metadata_pixel_1[0] * 256 + metadata_pixel_1[1];
  columns = metadata_pixel_1[2] * 256 + metadata_pixel_2[0];

  int total_pixels = host.rows * host.cols;
  Mat img_reveal(rows, columns, CV_8UC3);
  vector<int> perm_seq = generateLogisticMapSequence(total_pixels);

  int bit_counter = 0;

  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      Vec3b &pixel_r = img_reveal.at<Vec3b>(i, j);

      bitset<8> pixel_b, pixel_g, pixel_r_bits;

      for (int b = 0; b < 8; ++b) {
        if (bit_counter >= perm_seq.size()) {
          cerr << "Error: bit_counter excede el tamaño de perm_seq." << endl;
          exit(-1);
        }

        int permuted_index = perm_seq[bit_counter++];
        int px = permuted_index / host.cols;
        int py = permuted_index % host.cols;

        Vec3b pixel_h = host.at<Vec3b>(px, py);

        // Recuperar los bits de cada canal
        pixel_b[b] = pixel_h[0] & 1;
        pixel_g[b] = pixel_h[1] & 1;
        pixel_r_bits[b] = pixel_h[2] & 1;
      }

      // Reconstruimos los canales
      pixel_r[0] = (uchar)pixel_b.to_ulong();
      pixel_r[1] = (uchar)pixel_g.to_ulong();
      pixel_r[2] = (uchar)pixel_r_bits.to_ulong();
    }
  }
  return img_reveal;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "ERROR: No se envió imagen." << endl;
    cerr << "[./reveal.out]  [path-host]" << endl;
    return -1;
  }

  Mat img_host = imread(argv[1]);

  if (img_host.empty()) {
    cerr << "Error: No se pudo cargar la imagen host." << endl;
    return -1;
  }

  Mat img_reveal = revealImage(img_host);
  img_reveal = decryptImage(img_reveal);
  imwrite("./result/img_reveal.png", img_reveal);

  cout << "Imagen revelada guardada en ./result/img_reveal.png" << endl;

  return 0;
}
