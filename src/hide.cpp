#include <bitset>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include "opencv2/imgcodecs.hpp"

using namespace std;
using namespace cv;

Mat img_origin, img_host, img_hiden;
int rows, columns;
double x0 = 0.3;
double r = 3.9;
double a = 1.5;   // Constante -> Bernoulli Shift

void set_image(char *path_hosting, char *path_origin) {
  img_host = imread(path_hosting);
  img_origin = imread(path_origin);

  if (img_host.empty() || img_origin.empty()) {
    cerr << "Error: No se pudo cargar una de las imágenes." << endl;
    exit(-1);
  }

  cout << "Hosting: " << img_host.cols << " X " << img_host.rows << endl;
  cout << "Origin: " << img_origin.cols << " X " << img_origin.rows << endl;
  rows = img_origin.rows;
  columns = img_origin.cols;
}


vector<int> generateBernoulliSequence(int total_pixels) {
  vector<int> indices(total_pixels);

  for (int i = 0; i < total_pixels; ++i) {
    indices[i] = i;
  }

  // Usa el Bernoulli Shift para barajar la secuencia
  double x = x0;
  for (int i = 0; i < total_pixels; ++i) {
    x = fmod(a * x, 1.0);
    int j = static_cast<int>(x * total_pixels) % total_pixels;
    swap(indices[i], indices[j]);
  }

  return indices;
}

Mat encryptImage() {
  int total_pixels = rows * columns;
  Mat img_encrypted(rows, columns, CV_8UC3);
  vector<int> perm_seq = generateBernoulliSequence(total_pixels);

  for (int i = 0; i < total_pixels; ++i) {
    int x = i / columns;
    int y = i % columns;
    int permuted_index = perm_seq[i];
    int px = permuted_index / columns;
    int py = permuted_index % columns;
    img_encrypted.at<Vec3b>(px, py) = img_origin.at<Vec3b>(x, y);
  }
  return img_encrypted;
}

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

Mat hideImage(Mat host, Mat origin) {
  int total_pixels = host.rows * host.cols;
  Mat img_hide = host.clone();
  vector<int> perm_seq = generateLogisticMapSequence(total_pixels);

  Vec3b metadata_pixel;
  metadata_pixel[0] = rows / 256;
  metadata_pixel[1] = rows % 256;
  metadata_pixel[2] = columns / 256;
  img_hide.at<Vec3b>(0, 0) = metadata_pixel;

  metadata_pixel[0] = columns % 256;
  metadata_pixel[1] = 0;
  metadata_pixel[2] = 0;
  img_hide.at<Vec3b>(0, 1) = metadata_pixel;

  int bit_counter = 0;

  for (int i = 0; i < origin.rows; ++i) {
    for (int j = 0; j < origin.cols; ++j) {
      Vec3b pixel_o = origin.at<Vec3b>(i, j);

      // Extraemos los bits de cada canal del píxel origen ()RGb
      bitset<8> pixel_b(pixel_o[0]);
      bitset<8> pixel_g(pixel_o[1]);
      bitset<8> pixel_r(pixel_o[2]);

      for (int b = 0; b < 8; ++b) {
        if (bit_counter >= perm_seq.size()) {
          cerr << "Error: bit_counter excede el tamaño de perm_seq." << endl;
          exit(-1);
        }

        // Obtener la posición en el host usando perm_seq
        int permuted_index = perm_seq[bit_counter++];
        int px = permuted_index / host.cols;
        int py = permuted_index % host.cols;

        Vec3b &pixel_h = img_hide.at<Vec3b>(px, py);

        // Insertar los bits del canal azul, verde y rojo
        pixel_h[0] = (pixel_h[0] & 0xFE) | pixel_b[b];
        pixel_h[1] = (pixel_h[1] & 0xFE) | pixel_g[b];
        pixel_h[2] = (pixel_h[2] & 0xFE) | pixel_r[b];
      }
    }
  }
  return img_hide;
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
  if (argc != 3) {
    cerr << "ERROR: No se envió imágenes." << endl;
    cerr << "[./Example.out]  [path-host]  [path-origin]" << endl;
    return -1;
  }

  set_image(argv[1], argv[2]);

  Mat img_origin_encrypted = encryptImage();
  imwrite("./result/img_encrypted.png", img_origin_encrypted);

  img_hiden = hideImage(img_host, img_origin_encrypted);
  imwrite("./result/img_hiden.png", img_hiden);

  // Mat tmp = imread("./result/img_hiden.png");

  // Mat img_reveal = revealImage(tmp);
  // imwrite("./result/img_reveal.png", img_reveal);

  return 0;
}
