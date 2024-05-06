// GIF642 - Laboratoire - Mémoire partagée inter-processus
// Prépare un espace mémoire partagé et accessible depuis un script Python.
// La synchronisation est effectuée par envoi/réception de messages sur
// stdin/out.
// Messages de diagnostic sur stderr.
// Voir le script associé "waveprop/lab1_ex4.py".
#include <iostream>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <thread>

// Taille de la matrice de travail (un côté)
static const int MATRIX_SIZE = 100;
static const int BUFFER_SIZE = MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE * 3 * sizeof(double);
//static const int FUNCTION_VAR_BUFFER = 3*sizeof(double);
// Tampon générique à utiliser pour créer le fichier
char buffer_[BUFFER_SIZE];
char buffer_2[BUFFER_SIZE];

void wait_signal()
{
    // Attend une entrée (ligne complète avec \n) sur stdin.
    std::string msg;
    std::cin >> msg;
    //std::cerr << "CPP: Got signal." << std::endl;
}

void ack_signal()
{
    // Répond avec un message vide.
    std::cout << "" << std::endl;
}

double* curl_E(const double* E, double* curl_E, double courant_valeur) 
{
    int mat = MATRIX_SIZE;
    int x = 0;
    int y = 1;
    int z = 2;

    //std::cerr << "Begin curlE" << std::endl;
    std::thread t1([&](){
        for (int i = 0; i< mat - 1; i++) {
            for (int j = 0; j < mat; j++) {
                for (int k = 0; k < mat; k++) {
                curl_E[i*(mat^3) + j*(mat^2) + k*mat + y] = courant_valeur * (curl_E[i*(mat^3) + j*(mat^2) + k*mat + y] \
                                                                             - E[(i + 1)*(mat^3) + j*(mat^2) + k*mat + z] \
                                                                             + E[i*(mat^3) + j*(mat^2) + k*mat + z]);
                curl_E[i*(mat^3) + j*(mat^2) + k*mat + z] = courant_valeur * (curl_E[i*(mat^3) + j*(mat^2) + k*mat + z] \
                                                                             + E[(i + 1)*(mat^3) + j*(mat^2) + k*mat + y] \
                                                                             - E[i*(mat^3) + j*(mat^2) + k*mat + y]); 
                }
            }
        }
    });
    //std::cerr << "set1 curlE done" << std::endl;
    std::thread t2([&](){
        for (int i = 0; i< mat; i++) {
            for (int j = 0; j < mat - 1; j++) {
                for (int k = 0; k < mat; k++) {
                curl_E[i*(mat^3) + j*(mat^2) + k*mat + x] = courant_valeur * (curl_E[i*(mat^3) + j*(mat^2) + k*mat + x] \
                                                                                + E[i*(mat^3) + (j + 1)*(mat^2) + k*mat + z] \
                                                                                - E[i*(mat^3) + j*(mat^2) + k*mat + z]); 
                curl_E[i*(mat^3) + j*(mat^2) + k*mat + z] = courant_valeur * (curl_E[i*(mat^3) + j*(mat^2) + k*mat + z] \
                                                                                - E[i*(mat^3) + (j + 1)*(mat^2) + k*mat + x] \
                                                                                + E[i*(mat^3) + j*(mat^2) + k*mat + x]);
                }
            }   
        }
    });
    std::thread t3([&](){
        for (int i = 0; i< mat; i++) {
            for (int j = 0; j < mat; j++) {
                for (int k = 0; k < mat - 1; k++) {
                curl_E[i*(mat^3) + j*(mat^2) + k*mat + x] = courant_valeur * (curl_E[i*(mat^3) + j*(mat^2) + k*mat + x] \
                                                                                - E[i*(mat^3) + j*(mat^2) + (k + 1)*mat + y] \
                                                                                + E[i*(mat^3) + j*(mat^2) + k*mat + y]);
                curl_E[i*(mat^3) + j*(mat^2) + k*mat + y] = courant_valeur * (curl_E[i*(mat^3) + j*(mat^2) + k*mat + y] \
                                                                                + E[i*(mat^3) + j*(mat^2) + (k + 1)*mat + x] \
                                                                                - E[i*(mat^3) + j*(mat^2) + k*mat + x]); 
                }
            }
        }
    });
    t1.join();
    t2.join();
    t3.join();
    return curl_E;
}

double* curl_H(const double* H, double* curl_H, double courant_valeur)
{
    int mat = MATRIX_SIZE;
    int x = 0;
    int y = 1;
    int z = 2;

    std::thread t1([&](){
       for (int i = 0; i< mat - 1; i++) {
        for (int j = 0; j < mat; j++) {
            for (int k = 0; k < mat; k++) {
                curl_H[(i + 1)*(mat^3) + j*(mat^2) + k*mat + y] = courant_valeur * (curl_H[(i + 1)*(mat^3) + j*(mat^2) + k*mat + y] \
                                                                                    - H[(i + 1)*(mat^3) + j*(mat^2) + k*mat + z] \
                                                                                    + H[(i)*(mat^3) + j*(mat^2) + k*mat + z]);
                curl_H[(i + 1)*(mat^3) + j*(mat^2) + k*mat + z] = courant_valeur * (curl_H[(i + 1)*(mat^3) + j*(mat^2) + k*mat + z] \
                                                                                    + H[(i + 1)*(mat^3) + j*(mat^2) + k*mat + y] \
                                                                                    - H[(i)*(mat^3) + j*(mat^2) + k*mat + y]);
            }
        }
    } 
    });
    std::thread t2([&](){
        for (int i = 0; i < mat; i++) {
            for (int j = 0; j < mat - 1; j++) {
                for (int k = 0; k < mat; k++) {
                    curl_H[i*(mat^3) + (j + 1)*(mat^2) + k*mat + x] = courant_valeur * (curl_H[i*mat^3 + (j + 1)*mat^2 + k*mat + x] \
                                                                                        + H[i*(mat^3) + (j + 1)*(mat^2) + k*mat + z] \
                                                                                        - H[i*(mat^3) + (j)*(mat^2) + k*mat + z]);
                    curl_H[i*(mat^3) + (j + 1)*(mat^2) + k*mat + z] = courant_valeur * (curl_H[i*(mat^3) + (j + 1)*(mat^2) + k*mat + z] \
                                                                                        - H[i*(mat^3) + (j + 1)*(mat^2) + k*mat + x] \
                                                                                        + H[i*(mat^3) + (j)*(mat^2) + k*mat + x]);
                }
            }
        }
    });
    std::thread t3([&](){
        for (int i = 0; i < mat; i++) {
            for (int j = 0; j < mat; j++) {
                for (int k = 0; k < mat - 1; k++) {
                    curl_H[i*(mat^3) + j*(mat^2) + (k + 1)*mat + y] = courant_valeur * (curl_H[i*(mat^3) + j*(mat^2) + (k + 1)*mat + y]\
                                                                                        + H[i*(mat^3) + j*(mat^2) + (k + 1)*mat + x]\
                                                                                        - H[i*(mat^3) + j*(mat^2) + (k)*mat + x]);
                    curl_H[i*(mat^3) + j*(mat^2) + (k + 1)*mat + x] = courant_valeur * (curl_H[i*(mat^3) + j*(mat^2) + (k + 1)*mat + x]\
                                                                                        - H[i*(mat^3) + j*(mat^2) + (k + 1)*mat + y]\
                                                                                        + H[i*(mat^3) + j*(mat^2) + (k)*mat + y]);
                }
            }
        }
    });
    t1.join();
    t2.join();
    t3.join();
    return curl_H;
}

double* sum_Array(double* dest, double* content,bool addition=true)
{
    int mat = MATRIX_SIZE;
    int mult = addition ? 1 : -1;
    for (int i = 0; i < mat; i++) {
        for (int j = 0; j < mat; j++) {
            for (int k = 0; k < mat; k++) {
                for (int l = 0; k < 3; k++) {
                    dest[i*(mat^3) + j*(mat^2) + (k + 1)*mat + l] += mult*content[i*(mat^3) + j*(mat^2) + (k + 1)*mat + l];
                    if ( dest[i*(mat^3) + j*(mat^2) + (k + 1)*mat + l] != 0)
                    {
                        std::cerr << "Valeur qui saffiche pas en python : " << dest[i*(mat^3) + j*(mat^2) + (k + 1)*mat + l] << std::endl;
                        std::cerr << "A la position  x : " << i << ", " << j << ", "<< k << ", "<< l << std::endl;
                    }
                }   
            }
        }
    }
    return dest;
}

void calcul(double* E, double*& H, const double courant_number,const int source_pos[3], const double source_val)
{
    double* curl_E_ = new double[MATRIX_SIZE*MATRIX_SIZE*MATRIX_SIZE*3]{0};
    double* curl_H_ = new double[MATRIX_SIZE*MATRIX_SIZE*MATRIX_SIZE*3]{0};
    curl_H_ = curl_H(H,curl_H_,courant_number);
    sum_Array(E,curl_H_);
    E[source_pos[0]*(MATRIX_SIZE^3) + source_pos[1]*(MATRIX_SIZE^2) + source_pos[2]*MATRIX_SIZE] += source_val;

    curl_E_ = curl_E(E,curl_E_,courant_number);
    sum_Array(H,curl_E_,false);
    delete[] curl_E_;
    delete[] curl_H_;
}



int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Error : no shared file provided as argv[1]" << std::endl;
        return -1;
    }

    wait_signal();

    // Création d'un fichier "vide" (le fichier doit exister et être d'une
    // taille suffisante avant d'utiliser mmap).
    memset(buffer_, 0, BUFFER_SIZE);
    memset(buffer_2, 0, BUFFER_SIZE);
    FILE* shm_f = fopen(argv[1], "w");
    FILE* shm_f2 = fopen(argv[2], "w");
    fwrite(buffer_, sizeof(char), BUFFER_SIZE, shm_f);
    fwrite(buffer_2, sizeof(char), BUFFER_SIZE, shm_f2);
    fclose(shm_f);
    fclose(shm_f2);

    // On signale que le fichier est prêt.
    std::cerr << "CPP:  File ready." << std::endl;
    ack_signal();

    // On ré-ouvre le fichier et le passe à mmap(...). Le fichier peut ensuite
    // être fermé sans problèmes (mmap y a toujours accès, jusqu'à munmap.)
    int shm_fd = open(argv[1], O_RDWR);
    int shm_fd2 = open(argv[2], O_RDWR);
    void* shm_mmap = mmap(NULL, BUFFER_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    void* shm_mmap2 = mmap(NULL, BUFFER_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd2, 0);
    close(shm_fd);
    close(shm_fd2);

    if (shm_mmap == MAP_FAILED || shm_mmap2 == MAP_FAILED /*|| shm_mmap3 == MAP_FAILED*/) {
        std::cerr << "ERROR SHM\n";
        perror(NULL);
        return -1;
    }

    // Pointeur format double qui représente la matrice partagée:
    double* mtx = (double*)shm_mmap;
    double* mtx2 = (double*)shm_mmap2;

    int index = 0;
    const int source_pos[3] = {(int)MATRIX_SIZE/3,(int)MATRIX_SIZE/3,(int)MATRIX_SIZE/2};
    const int courant_valeur = 0.1;

    while (true) {

        // On attend le signal du parent.
        wait_signal();
        //migré du main
        // On fait le travail.
        calcul(mtx,mtx2,courant_valeur,source_pos,0.1*sin(0.1*index));
        index++;
        // On signale que le travail est terminé.
        //std::cerr << "CPP: Work done." << std::endl;
        ack_signal();
    }

    munmap(shm_mmap, BUFFER_SIZE);
    munmap(shm_mmap2, BUFFER_SIZE);
    return 0;
}