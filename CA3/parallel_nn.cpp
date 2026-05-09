#include <pthread.h>
#include <semaphore.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>

using namespace std;
using namespace std::chrono;

#define MNIST_TESTING_SET_IMAGE_FILE_NAME "data/t10k-images-idx3-ubyte"
#define MNIST_TESTING_SET_LABEL_FILE_NAME "data/t10k-labels-idx1-ubyte"

const int NUMBER_OF_INPUT_CELLS = 784;

int NUMBER_OF_HIDDEN_CELLS = 256;
int NUMBER_OF_OUTPUT_CELLS = 10;
int NUMBER_OF_HIDDEN_THREADS = 8;
int NUMBER_OF_OUTPUT_THREADS = 10;

struct HiddenNode {
    vector<double> weights;
    double bias;
};

struct OutputNode {
    vector<double> weights;
    double bias;
};

vector<HiddenNode> hidden_nodes;
vector<OutputNode> output_nodes;
vector<double> hidden_layer_outputs;
vector<double> output_layer_outputs;

sem_t sem_hidden;
sem_t sem_output;

// MNIST reading functions
FILE* openMNISTImageFile(const char* fileName) {
    FILE* f = fopen(fileName, "rb");
    if (!f) { perror("Cannot open image file"); exit(1); }
    fseek(f, 16, SEEK_SET);
    return f;
}

FILE* openMNISTLabelFile(const char* fileName) {
    FILE* f = fopen(fileName, "rb");
    if (!f) { perror("Cannot open label file"); exit(1); }
    fseek(f, 8, SEEK_SET);
    return f;
}

void getImage(FILE* f, uint8_t* pixels) {
    if (fread(pixels, 1, NUMBER_OF_INPUT_CELLS, f) != NUMBER_OF_INPUT_CELLS) {
        perror("Read image");
        exit(1);
    }
}

uint8_t getLabel(FILE* f) {
    uint8_t lbl;
    if (fread(&lbl, 1, 1, f) != 1) {
        perror("Read label");
        exit(1);
    }
    return lbl;
}

// Allocate weights and biases
void allocateParameters() {
    hidden_nodes.resize(NUMBER_OF_HIDDEN_CELLS);
    hidden_layer_outputs.resize(NUMBER_OF_HIDDEN_CELLS);
    output_nodes.resize(NUMBER_OF_OUTPUT_CELLS);
    output_layer_outputs.resize(NUMBER_OF_OUTPUT_CELLS);

    // Hidden layer parameters
    ifstream hW("net_params/hidden_weights.txt");
    for (int i = 0; i < NUMBER_OF_HIDDEN_CELLS; ++i) {
        hidden_nodes[i].weights.resize(NUMBER_OF_INPUT_CELLS);
        for (int j = 0; j < NUMBER_OF_INPUT_CELLS; ++j) {
            hW >> hidden_nodes[i].weights[j];
        }
    }
    hW.close();

    ifstream hB("net_params/hidden_biases.txt");
    for (int i = 0; i < NUMBER_OF_HIDDEN_CELLS; ++i) {
        hB >> hidden_nodes[i].bias;
    }
    hB.close();

    // Output layer parameters
    ifstream oW("net_params/out_weights.txt");
    for (int i = 0; i < NUMBER_OF_OUTPUT_CELLS; ++i) {
        output_nodes[i].weights.resize(NUMBER_OF_HIDDEN_CELLS);
        for (int j = 0; j < NUMBER_OF_HIDDEN_CELLS; ++j) {
            oW >> output_nodes[i].weights[j];
        }
    }
    oW.close();

    ifstream oB("net_params/out_biases.txt");
    for (int i = 0; i < NUMBER_OF_OUTPUT_CELLS; ++i) {
        oB >> output_nodes[i].bias;
    }
    oB.close();
}

// Parallel hidden layer computation
struct HiddenThreadArg { int start, end; uint8_t* pixels; };
void* hiddenThreadFunc(void* arg) {
    HiddenThreadArg* a = (HiddenThreadArg*)arg;
    for (int i = a->start; i < a->end; ++i) {
        double sum = 0;
        for (int j = 0; j < NUMBER_OF_INPUT_CELLS; ++j) {
            sum += a->pixels[j] * hidden_nodes[i].weights[j];
        }
        sum += hidden_nodes[i].bias;
        hidden_layer_outputs[i] = sum > 0 ? sum : 0; // ReLU
    }
    sem_post(&sem_hidden);
    return NULL;
}


void parallelHidden(uint8_t* pixels) {
    sem_init(&sem_hidden, 0, 0);
    pthread_t th[NUMBER_OF_HIDDEN_THREADS];
    HiddenThreadArg args[NUMBER_OF_HIDDEN_THREADS];
    int chunk = NUMBER_OF_HIDDEN_CELLS / NUMBER_OF_HIDDEN_THREADS;
    for (int t = 0; t < NUMBER_OF_HIDDEN_THREADS; ++t) {
        args[t].start = t * chunk;
        args[t].end = (t == NUMBER_OF_HIDDEN_THREADS - 1) ? NUMBER_OF_HIDDEN_CELLS : (t + 1) * chunk;
        args[t].pixels = pixels;
        pthread_create(&th[t], NULL, hiddenThreadFunc, &args[t]);
    }
    for (int t = 0; t < NUMBER_OF_HIDDEN_THREADS; ++t) sem_wait(&sem_hidden);
    for (int t = 0; t < NUMBER_OF_HIDDEN_THREADS; ++t) pthread_join(th[t], NULL);
    sem_destroy(&sem_hidden);
}

// Parallel output layer computation
struct OutputThreadArg { int idx; };
void* outputThreadFunc(void* arg) {
    OutputThreadArg* a = (OutputThreadArg*)arg;
    int i = a->idx;
    double sum = 0;
    for (int j = 0; j < NUMBER_OF_HIDDEN_CELLS; ++j) {
        sum += hidden_layer_outputs[j] * output_nodes[i].weights[j];
    }
    sum += output_nodes[i].bias;
    output_layer_outputs[i] = 1 / (1 + exp(-sum)); // Sigmoid
    sem_post(&sem_output);
    return NULL;
}

void parallelOutput() {
    sem_init(&sem_output, 0, 0);
    pthread_t th[NUMBER_OF_OUTPUT_THREADS];
    OutputThreadArg args[NUMBER_OF_OUTPUT_THREADS];
    for (int i = 0; i < NUMBER_OF_OUTPUT_THREADS; ++i) {
        args[i].idx = i;
        pthread_create(&th[i], NULL, outputThreadFunc, &args[i]);
    }
    for (int i = 0; i < NUMBER_OF_OUTPUT_THREADS; ++i) sem_wait(&sem_output);
    for (int i = 0; i < NUMBER_OF_OUTPUT_THREADS; ++i) pthread_join(th[i], NULL);
    sem_destroy(&sem_output);
}

// Prediction
int getPrediction() {
    int best = 0;
    double maxv = output_layer_outputs[0];
    for (int i = 1; i < NUMBER_OF_OUTPUT_CELLS; ++i) {
        if (output_layer_outputs[i] > maxv) {
            maxv = output_layer_outputs[i];
            best = i;
        }
    }
    return best;
}

// Test NN
void testNN() {
    FILE* imgF = openMNISTImageFile(MNIST_TESTING_SET_IMAGE_FILE_NAME);
    FILE* lblF = openMNISTLabelFile(MNIST_TESTING_SET_LABEL_FILE_NAME);
    uint8_t pixels[NUMBER_OF_INPUT_CELLS];
    int errors = 0;

    auto start = high_resolution_clock::now();

    for (int n = 0; n < 10000; ++n) {
        getImage(imgF, pixels);
        uint8_t lbl = getLabel(lblF);
        parallelHidden(pixels);
        parallelOutput();
        int pred = getPrediction();
        if (pred != lbl) errors++;
        printf("Image %d => Pred: %d, Act: %d\n", n + 1, pred, lbl);
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    printf("\nTotal Execution Time: %lld ms\n", duration.count());

    printf("\nAccuracy: %.2f%%\n", 100.0 * (10000 - errors) / 10000.0);
    fclose(imgF);
    fclose(lblF);
}

int main() {
    std::cout << "Enter number of hidden layer neurons (max 256): ";
    std::cin >> NUMBER_OF_HIDDEN_CELLS;
    if (NUMBER_OF_HIDDEN_CELLS <= 0 || NUMBER_OF_HIDDEN_CELLS > 256) NUMBER_OF_HIDDEN_CELLS = 256;

    std::cout << "Enter number of output layer neurons (max 10): ";
    std::cin >> NUMBER_OF_OUTPUT_CELLS;
    if (NUMBER_OF_OUTPUT_CELLS <= 0 || NUMBER_OF_OUTPUT_CELLS > 10) NUMBER_OF_OUTPUT_CELLS = 10;

    std::cout << "Enter number of threads for hidden layer: ";
    std::cin >> NUMBER_OF_HIDDEN_THREADS;
    if (NUMBER_OF_HIDDEN_THREADS <= 0) NUMBER_OF_HIDDEN_THREADS = 8;

    std::cout << "Enter number of threads for output layer: ";
    std::cin >> NUMBER_OF_OUTPUT_THREADS;
    if (NUMBER_OF_OUTPUT_THREADS <= 0) NUMBER_OF_OUTPUT_THREADS = 10;

    allocateParameters();
    testNN();
    return 0;
}
