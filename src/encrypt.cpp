#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

using namespace std;
using namespace cv;

/* Variables globales */
Mat img_origin, img_encrypted, img_decrypted;
int rows, columns;
double x0 = 0.3; // Valor inicial para el Bernoulli Shift
double a = 1.5;  // Constante para el Bernoulli Shift

void set_image(char *path_origin) {
  img_origin = imread(path_origin);
  if (img_origin.empty()) {
    cerr << "Error: Imagen no encontrada." << endl;
    exit(-1);
  }
  rows = img_origin.rows;
  columns = img_origin.cols;
  cout << "Tamaño de la imagen: " << columns << " x " << rows << endl;
}

vector<int> generateBernoulliSequence(int total_pixels) {
  vector<int> indices(total_pixels);

  for (int i = 0; i < total_pixels; ++i) {
    indices[i] = i;
  }

  // Usa el Bernoulli Shift para barajar la secuencia
  double x = x0;
  for (int i = 0; i < total_pixels; ++i) {
    x = fmod(a * x, 1.0); // Bernoulli Shift
    int j = static_cast<int>(x * total_pixels) %
            total_pixels;         // Índice pseudoaleatorio
    swap(indices[i], indices[j]); // Intercambia los elementos
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

int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "ERROR: No se envió la imagen." << endl;
    cerr << "Uso: ./programa [ruta-imagen]" << endl;
    return -1;
  }

  set_image(argv[1]);

  img_encrypted = encryptImage();
  imwrite("./result/img_encrypted.jpg", img_encrypted);

  img_decrypted = decryptImage(img_encrypted);
  imwrite("./result/img_decrypted.jpg", img_decrypted);

  return 0;
}
