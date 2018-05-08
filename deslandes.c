#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>

typedef struct Vecteur vecteur;

struct Vecteur{
    int taille;
    int* contenu;
};

typedef struct Matrice matrice;

struct Matrice{
    int nb_lignes;
    int nb_colonnes;
    int** lignes;
};

void affiche_vecteur(vecteur vec){
    printf("(");
    for(int i =0;i<vec.taille-1;i++){
        printf("%d,",vec.contenu[i]);
    }
    printf("%d)\n",vec.contenu[vec.taille-1]);
}

void affiche_matrice(matrice mat){
    printf("(");
    for (int i =0;i<mat.nb_lignes;i++){
        printf("(");
        for (int j=0;j<mat.nb_colonnes;j++){
            printf("%d,",mat.lignes[i][j]);
        }
        printf("),\n");
    }
    printf(")\n");
}

int* alloue_contenu_vecteur(int taille){
    int* contenu = (int*) malloc(taille*sizeof(int));
    for (int i =0;i<taille;i++){
        contenu[i] = 0;
    }
    return contenu;
}

int** alloue_contenu_matrice(int colonne, int ligne){
    int** contenu = (int**) malloc(ligne*sizeof(int*));
    for (int i =0;i<ligne;i++){
        contenu[i] = (int*) malloc(colonne*sizeof(int));
        for (int j =0;j<colonne;j++){
            contenu[i][j] = 0;
        }
    }
    return contenu;
}

int calcul_taille_matrice(char* nom_fichier){
    FILE* fichier = NULL;
    fichier = fopen(nom_fichier,"r");
    char caracter = fgetc(fichier);
    int taille_matrice = 1;
    while (caracter!='\n'){
        if (caracter == ' ') taille_matrice += 1;
        caracter = fgetc(fichier);
    }
    fclose(fichier);
    return taille_matrice;
}

int calcul_taille_vecteur(char* nom_fichier){
    FILE* fichier = NULL;
    fichier = fopen(nom_fichier,"r");
    char caracter = fgetc(fichier);
    int taille_vecteur = 0;
    while (caracter!=EOF){
        if (caracter == '\n') taille_vecteur += 1;
        caracter = fgetc(fichier);
    }
    fclose(fichier);
    return taille_vecteur;
}

void charge_matrice(char* nom_fichier, matrice* mat){
    int taille_matrice = calcul_taille_matrice(nom_fichier);
    mat->nb_lignes = taille_matrice;
    mat->nb_colonnes = taille_matrice;
    mat->lignes = alloue_contenu_matrice(taille_matrice,taille_matrice);
    FILE* fichier = NULL;
    fichier = fopen(nom_fichier,"r");
    char caracter = fgetc(fichier);
    int est_negatif = 0;
    int nb_colonne = 0;
    int nb_ligne = 0;
    while (caracter!=EOF){
        if (caracter=='-') est_negatif = 1;
        if (isdigit(caracter)){
            int nb = (int)caracter - 48;
            mat->lignes[nb_ligne][nb_colonne] = mat->lignes[nb_ligne][nb_colonne] * 10 + nb;
        }
        if (caracter == ' '){
            if (est_negatif){
                mat->lignes[nb_ligne][nb_colonne] = -1 * mat->lignes[nb_ligne][nb_colonne];
                est_negatif = 0;
            }
            nb_colonne += 1;
        }
        if (caracter == '\n'){
            if (est_negatif){
                mat->lignes[nb_ligne][nb_colonne] = -1 * mat->lignes[nb_ligne][nb_colonne];
                est_negatif = 0;
            }
            nb_colonne = 0;
            nb_ligne += 1;
        }
        caracter = fgetc(fichier);
    }
    fclose(fichier);
}

void charge_vecteur(char* nom_fichier,vecteur* vec){
    int taille_vecteur = calcul_taille_vecteur(nom_fichier);
    vec->taille = taille_vecteur;
    vec->contenu = alloue_contenu_vecteur(taille_vecteur);
    FILE* fichier = NULL;
    fichier = fopen(nom_fichier,"r");
    char caracter = fgetc(fichier);
    int est_negatif = 0;
    int nb_ligne = 0;
    while (caracter!=EOF){
        if (caracter=='-') est_negatif = 1;
        if (isdigit(caracter)){
            int nb = (int)caracter - 48;
            vec->contenu[nb_ligne] = vec->contenu[nb_ligne] * 10 + nb;
        }
        if (caracter == '\n'){
            if (est_negatif){
                vec->contenu[nb_ligne] = -1 * vec->contenu[nb_ligne];
                est_negatif = 0;
            }
            nb_ligne += 1;
        }
        caracter = fgetc(fichier);
    }
    fclose(fichier);
}

void transfert_nb_decoupe(int nb_decoupe, int nb_process){
    for (int i =1;i<nb_process;i++){
        MPI_Send(&nb_decoupe,1,MPI_INT,i,1,MPI_COMM_WORLD);
    }
}

int recoie_nb_decoupe(){
    int message;
    MPI_Recv(&message, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return message;
}

void transfert_taille_matrice_restante(int taille, int suivant){
    MPI_Send(&taille,1,MPI_INT,suivant,1,MPI_COMM_WORLD);
}

int recoie_taille_matrice_restante(int precedent){
    int to_return;
    MPI_Recv(&to_return,1,MPI_INT,precedent,MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return to_return;
}

void transfert_matrice_restante(matrice mat, int suivant){
    for (int i =0;i<mat.nb_lignes;i++){
        int* sous_mat = mat.lignes[i];
        MPI_Send(sous_mat,mat.nb_colonnes,MPI_INT,suivant,1,MPI_COMM_WORLD);
    }
}

matrice recoie_matrice_restante(int taille, int precedent, int nb_colonnes){
    matrice mat;
    mat.nb_colonnes = nb_colonnes;
    mat.nb_lignes = taille;
    mat.lignes = alloue_contenu_matrice(mat.nb_colonnes,mat.nb_lignes);
    for (int i =0;i<mat.nb_lignes;i++){
        int* reception = (int*) malloc(nb_colonnes*sizeof(int));
        MPI_Recv(reception,nb_colonnes,MPI_INT,precedent,MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        mat.lignes[i] = reception;
    }
    return mat;
}

void transfert_resultat(vecteur resultat, int suivant){
    int* res = resultat.contenu;
    MPI_Send(res,resultat.taille,MPI_INT,suivant,1,MPI_COMM_WORLD);
}

vecteur recoie_resultat(int taille_resultat,int precedent){
    vecteur to_return;
    to_return.taille = taille_resultat;
    int* reception = (int*) malloc(taille_resultat*sizeof(int));
    MPI_Recv(reception,taille_resultat,MPI_INT,precedent,MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    to_return.contenu = reception;
    return to_return;
}

void reduit_matrice_lignes(matrice* mat,int nb_decoupe){
    mat->nb_lignes -= nb_decoupe;
    for (int i = 0;i<nb_decoupe;i++){
        mat->lignes[i] = mat->lignes[i+nb_decoupe];
    }
}

matrice decoupe_matrice(matrice* mat, int nb_decoupe){
    matrice decoupe;
    decoupe.nb_colonnes = mat->nb_colonnes;
    decoupe.nb_lignes = nb_decoupe;
    decoupe.lignes = alloue_contenu_matrice(decoupe.nb_colonnes,decoupe.nb_lignes);
    for (int i =0;i<nb_decoupe;i++){
        for (int j =0;j<decoupe.nb_colonnes;j++){
            decoupe.lignes[i][j] = mat->lignes[i][j];
        }
    }
    reduit_matrice_lignes(mat,nb_decoupe);
    return decoupe;
}

int calcul_vecteur_vecteur(int* tab, vecteur vec){
    int resultat = 0;
    for (int i =0;i<vec.taille;i++){
        resultat += tab[i] * vec.contenu[i];
    }
    return resultat;
}

void calcul_resultat(matrice mat,vecteur vec,vecteur* resultat){
    int taille_precedente = resultat->taille;
    if (resultat->taille == 0){
        resultat->taille = mat.nb_lignes;
        resultat->contenu = alloue_contenu_vecteur(resultat->taille);
    }else{
        resultat->taille += mat.nb_lignes;
        resultat->contenu = realloc(resultat->contenu, resultat->taille*sizeof(int));
        for (int i = taille_precedente;i<resultat->taille;i++){
            resultat->contenu[i] = 0;
        }
    }
    int j = 0;
    for (int i =taille_precedente;i<taille_precedente+mat.nb_lignes;i++){
        resultat->contenu[i] = calcul_vecteur_vecteur(mat.lignes[j++],vec);
    }
}

void enregistre_resultat(vecteur resultat){
    FILE* fichier = NULL;
    fichier = fopen("resultat","w");
    for (int i=0;i<resultat.taille;i++){
        int nDigits = floor(log10(abs(resultat.contenu[i]))) + 1;
        char* rajoute = (char*) malloc(sizeof(char)*nDigits);
        sprintf(rajoute,"%d",resultat.contenu[i]);
        fputs(rajoute,fichier);
        fputs("\n",fichier);
    }
    fclose(fichier);
}

int main(int argc,char** argv){
    int rang;
    int nb_process;
    char nom_processeur[MPI_MAX_PROCESSOR_NAME];
    int longueur_nom;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_process);
    MPI_Get_processor_name(nom_processeur, &longueur_nom);

    int precedent = (rang+nb_process-1)%nb_process;
    int suivant = (rang+1)%nb_process;
    int taille_matrice_restante;
    int taille_resultat;

    matrice mat;
    vecteur vec;
    matrice sous_matrice;
    vecteur resultat;
    resultat.taille = 0;
    resultat.contenu = NULL;

    int nb_decoupe;

    if(rang==0){
        charge_matrice(argv[1],&mat);
        charge_vecteur(argv[2],&vec);
        nb_decoupe = mat.nb_lignes / nb_process;
        transfert_nb_decoupe(nb_decoupe,nb_process);
        sous_matrice = decoupe_matrice(&mat,nb_decoupe);
        transfert_taille_matrice_restante(mat.nb_lignes,suivant);
        transfert_matrice_restante(mat,suivant);
        calcul_resultat(sous_matrice,vec,&resultat);
        transfert_resultat(resultat,suivant);

        taille_resultat = vec.taille;
        resultat = recoie_resultat(taille_resultat,precedent);
        enregistre_resultat(resultat);
    }else{
        charge_vecteur(argv[2],&vec);
        nb_decoupe = recoie_nb_decoupe();
        taille_matrice_restante = recoie_taille_matrice_restante(precedent);
        mat = recoie_matrice_restante(taille_matrice_restante,precedent,vec.taille);
        taille_resultat = nb_decoupe*rang;

        sous_matrice = decoupe_matrice(&mat,nb_decoupe);
        resultat = recoie_resultat(taille_resultat,precedent);
        calcul_resultat(sous_matrice,vec,&resultat);
        if (suivant != 0){
            transfert_taille_matrice_restante(mat.nb_lignes,suivant);
            transfert_matrice_restante(mat,suivant);
        }
        transfert_resultat(resultat,suivant);
    }
    MPI_Finalize();
    return 0;
}
