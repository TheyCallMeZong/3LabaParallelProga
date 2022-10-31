#include <iostream>
#include <fstream>
#include <string>
#include "delegationmodel.h"
#include <omp.h>

using namespace std;
using namespace del;

void alghoritmEvclida(int x, int y);

int main() {
    ifstream input("../data.txt");
    float ch;
    int Count = 0;
    while (input >> ch)
        Count++;
    input.close();

    int *mass = new int[Count];
    ifstream file("../data.txt");
    for(int i=0; i<Count; i++) {
        file>>mass[i];
    }

    auto begin = std::chrono::steady_clock::now();
    DelegationModel pool;
    for( int i = 0; i < Count; i+=2 ) {
        cout << i << endl;
        pool.add_job([mass, i]() {
            cout << "Задача отпралвена\n";
            alghoritmEvclida(mass[i], mass[i + 1]);
        });
    }

    pool.join_all();
    auto end = std::chrono::steady_clock::now();

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    std::cout << "The time: " << elapsed_ms.count() << " ms\n";
    //готово
}

void alghoritmEvclida(int x, int y){
    string file_name = "../" + to_string(x) + to_string(y) + ".txt";
    while (x != y) {
        if (x>y) {
            x = x-y;
        }
        else {
            y = y-x;
        }
    }
    ofstream out;
    out.open(file_name);
    if (out.is_open())
    {
        out << x << std::endl;
    }
    out.close();
}