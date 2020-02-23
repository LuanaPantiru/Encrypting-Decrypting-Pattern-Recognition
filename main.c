#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
typedef struct
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
} RGB;
unsigned char* header(char *nume_fisier,unsigned int *w_img,unsigned int *h_img)
{
    FILE *f=fopen(nume_fisier,"rb");
    if(f==NULL)
    {
        printf("Nu s-a deschis fisierul pentru a se putea determina dimensiunile imaginii");
        exit(-1);
    }
    unsigned char *aux;
    fseek(f,18,SEEK_SET);
    fread(&(*w_img),sizeof(unsigned int),1,f);
    fread(&(*h_img),sizeof(unsigned int),1,f);
    fseek(f,0,SEEK_SET);
    aux=malloc(54);
    for(int i=0; i<54; i++)
        fread(&aux[i],sizeof(unsigned char),1,f);
    fclose(f);
    return aux;
}
unsigned int padding(unsigned int w_img)
{
    if(w_img%4!=0)
        return 4-(3*w_img)%4;
    else
        return 0;

}
unsigned int xorshift32(unsigned int seed)
{
    unsigned int r;
    r=seed;
    r=r^r<<13;
    r=r^r>>17;
    r=r^r<<5;
    return r;
}
void liniarizare(char *nume_imagine,RGB **L)
{
    FILE *fin;
    fin=fopen(nume_imagine,"rb");
    if(fin==NULL)
    {
        printf("Nu s-a deschis fisierul pentru citire");
        exit(-1);
    }
    unsigned char *aux;
    unsigned int w_img, h_img;
    aux=header(nume_imagine,&w_img,&h_img);                 ///memoram header-ul si aflam inaltimea si latimea imaginii
    unsigned int pad=padding(w_img);                        ///calculam padding-ul imaginii
    fseek(fin,54,SEEK_SET);
    *L=(RGB*)malloc(w_img*h_img*sizeof(RGB));               ///inceperea liniarizarii imaginii
    unsigned int k;
    for(int i=0; i<h_img; i++)
    {
        k=(h_img-i-1)*w_img;
        for(int j=0; j<w_img; j++)
        {
            fread(&(*L)[k].B,sizeof(unsigned char),1,fin);
            fread(&(*L)[k].G,sizeof(unsigned char),1,fin);
            fread(&(*L)[k].R,sizeof(unsigned char),1,fin);
            k++;
        }
        fseek(fin,pad,SEEK_CUR);                            ///cand intalnim padding-ul il sarim pentru a nu fi pus in liniarizare
    }
    free(aux);

}
void afisare(char *nume_imagine,RGB *L,unsigned char *aux,unsigned int w_img,unsigned int h_img)
{
    FILE *fout=fopen(nume_imagine,"wb");
    if(fout==NULL)
    {
        printf("Nu s-a deschis fisierul pentru scriere");
        exit(-1);
    }
    unsigned int pad=padding(w_img);                        ///calculam padding-ul
    for(int i=0; i<54; i++)                                 ///afisam header-ul imaginii
        fwrite(&aux[i],sizeof(unsigned char),1,fout);
    unsigned int k;
    unsigned int p=0;
    for(int i=0; i<h_img; i++)                              ///incepem afisarea vectoruluii liniarizat
    {
        k=(h_img-i-1)*w_img;
        for(int j=0; j<w_img; j++)
        {
            fwrite(&L[k].B,sizeof(unsigned char),1,fout);
            fwrite(&L[k].G,sizeof(unsigned char),1,fout);
            fwrite(&L[k].R,sizeof(unsigned char),1,fout);
            k++;
        }
        for(int l=0; l<pad; l++)                            ///afisam padding-ul
        {
            fwrite(&p,1,1,fout);
            fflush(fout);
        }
    }
    fclose(fout);
}
void swap(unsigned int *a,unsigned int *b)
{
    unsigned int aux;
    aux=(*a);
    *a=*b;
    *b=aux;
}
void criptare(char *nume_imagine,char *img_criptata, char *nume_fisier)
{
    unsigned int w_img,h_img;
    unsigned char *aux;
    RGB *L;
    liniarizare(nume_imagine,&L);                           ///aflam datele legate de imagine,liniarizarea sa,header-ul si dimensiunile
    aux=header(nume_imagine,&w_img,&h_img);
    unsigned int *R;
    R=(unsigned int*)malloc((2*w_img*h_img-1)*sizeof(unsigned int));
    unsigned int SV;
    FILE *f=fopen(nume_fisier,"r");                         ///cititm cheile secrete
    fscanf(f,"%u",&R[0]);
    fscanf(f,"%u",&SV);
    fclose(f);
    for(int i=1; i<=2*w_img*h_img-1; i++)                   ///determinam numerele random dupa cheia secreta
        R[i]=xorshift32(R[i-1]);
    unsigned int *v;
    v=(unsigned int*)malloc(w_img*h_img*sizeof(unsigned int));
    for(int i=0; i<w_img*h_img; i++)                        ///realizam vectorul de permutari
        v[i]=i;
    int r,j=1;
    for(int i=w_img*h_img-1; i>=0; i--)
    {
        r=R[j]%(i+1);
        swap(&v[i],&v[r]);
        j++;
    }
    RGB *L1;
    L1=(RGB*)malloc(w_img*h_img*sizeof(RGB));               ///realizarea imaginii intermediare folosind vectorul de permutari
    for(int i=0; i<w_img*h_img; i++)
        L1[v[i]]=L[i];
    RGB *C;
    C=(RGB*)malloc(w_img*h_img*sizeof(RGB));                ///realizam imaginea criptata
    j=w_img*h_img;
    for(int i=0; i<w_img*h_img; i++)
    {
        switch(i)
        {
        case 0:
        {
            C[0].B=(L1[0].B)^(SV&0xFF)^(R[j]&0xFF);
            C[0].G=(L1[0].G)^((SV>>8)&0xFF)^((R[j]>>8)&0xFF);
            C[0].R=(L1[0].R)^((SV>>16)&0xFF)^((R[j]>>16)&0xFF);
            j++;
        }
        break;
        default:
        {
            C[i].B=(C[i-1].B)^(L1[i].B)^(R[j]&0xFF);
            C[i].G=(C[i-1].G)^(L1[i].G)^((R[j]>>8)&0xFF);
            C[i].R=(C[i-1].R)^(L1[i].R)^((R[j]>>16)&0xFF);
            j++;
        }
        break;
        }
    }
    afisare(img_criptata,C,aux,w_img,h_img);                ///afisam imaginea criptata
    free(L);
    free(L1);
    free(R);
    free(v);
    free(C);
    free(aux);
    printf("CRIPTARE REUSITA\n");
}
void decriptare(char *img_initiala,char *img_criptata,char *nume_fisier)
{
    unsigned int w_img,h_img;
    unsigned char *aux;
    RGB *C;
    liniarizare(img_criptata,&C);                           ///aflam datele imaginii criptate,liniarizarea,header-ul si dimensiunile
    aux=header(img_criptata,&w_img,&h_img);
    unsigned int *R;
    R=(unsigned int*)malloc((2*w_img*h_img-1)*sizeof(unsigned int));
    unsigned int SV;
    FILE *f=fopen(nume_fisier,"r");                         ///citim cheile secrete
    if(f==NULL)
    {
        printf("Nu s-a deschis fisierul cu cheia secreta");
        exit(-1);
    }
    fscanf(f,"%u",&R[0]);
    fscanf(f,"%u",&SV);
    fclose(f);
    for(int i=1; i<=2*w_img*h_img-1; i++)                   ///aflam numerele random
        R[i]=xorshift32(R[i-1]);
    unsigned int *v;
    v=(unsigned int*)malloc(w_img*h_img*sizeof(unsigned int));
    for(int i=0; i<w_img*h_img; i++)                        ///realizam vectorul de permutari
        v[i]=i;
    int r,j=1;
    for(int i=w_img*h_img-1; i>=0; i--)
    {
        r=R[j]%(i+1);
        swap(&v[i],&v[r]);
        j++;
    }
    RGB *C1;
    C1=(RGB*)malloc(w_img*h_img*sizeof(RGB));               ///determinam imaginea intermediara
    j=w_img*h_img;
    for(int i=0; i<w_img*h_img; i++)
    {
        switch(i)
        {
        case 0:
        {
            C1[0].B=(C[0].B)^(SV&0xFF)^(R[j]&0xFF);
            C1[0].G=(C[0].G)^((SV>>8)&0xFF)^((R[j]>>8)&0xFF);
            C1[0].R=(C[0].R)^((SV>>16)&0xFF)^((R[j]>>16)&0xFF);
            j++;
        }
        break;
        default:
        {
            C1[i].B=(C[i-1].B)^(C[i].B)^(R[j]&0xFF);
            C1[i].G=(C[i-1].G)^(C[i].G)^((R[j]>>8)&0xFF);
            C1[i].R=(C[i-1].R)^(C[i].R)^((R[j]>>16)&0xFF);
            j++;
        }
        break;
        }
    }
    RGB *D;
    D=(RGB*)malloc(w_img*h_img*sizeof(RGB));                ///decriptam imaginea
    for(int i=0; i<w_img*h_img; i++)
        D[i]=C1[v[i]];
    afisare(img_initiala,D,aux,w_img,h_img);                ///afisam imaginea decriptata
    free(C);
    free(aux);
    free(C1);
    free(R);
    free(v);
    free(D);
    printf("DECRIPTARE REUSITA\n");
}
void chi_patrat(char *imagine)
{
    unsigned int *fR,*fG,*fB,w_img,h_img;
    RGB *L;
    unsigned char *aux;
    fR=(unsigned int*)malloc(256*sizeof(unsigned int));     ///initializam vectorii pentru frecventa celor 3 culori
    fG=(unsigned int*)malloc(256*sizeof(unsigned int));
    fB=(unsigned int*)malloc(256*sizeof(unsigned int));
    for(int i=0; i<=255; i++)
    {
        fR[i]=0;
        fG[i]=0;
        fB[i]=0;
    }
    liniarizare(imagine,&L);                                ///aflam datele despre imagine,liniarizarea,header-ul si dimensiunile
    aux=header(imagine,&w_img,&h_img);
    for(int i=0; i<w_img*h_img; i++)                        ///parcurgem imaginea si crestem frecventa culorilor
    {
        fR[L[i].R]++;
        fG[L[i].G]++;
        fB[L[i].B]++;
    }
    float x2R=0,x2G=0,x2B=0,f;
    f=w_img*h_img/256;                                      ///calculam chi-patratul
    for(int i=0; i<=255; i++)
    {
        x2R=x2R+(fR[i]-f)*(fR[i]-f)/f;
        x2G=x2G+(fG[i]-f)*(fG[i]-f)/f;
        x2B=x2B+(fB[i]-f)*(fB[i]-f)/f;
    }
    printf("R:%.2f\nG:%.2f\nB:%.2f\n",x2R,x2G,x2B);
}
void matrice(char *imagine,RGB ***mat)
{
    unsigned char *aux;
    unsigned int w_img,h_img;
    aux=header(imagine,&w_img,&h_img);
    FILE *fin=fopen(imagine,"rb");
    unsigned int pad=padding(w_img);                        ///se calculeaza padding-ul
    (*mat)=(RGB**)malloc(h_img*sizeof(RGB*));
    fseek(fin,54,SEEK_SET);                                 ///imaginea se momoreaza intr-o matrice, primul element din matrice fiind pixelul din stanga jos al imaginii
    for(int i=0; i<h_img; i++)
    {
        (*mat)[i]=(RGB*)malloc(w_img*sizeof(RGB));
        for(int j=0; j<w_img; j++)
        {
            fread(&(*mat)[i][j].B,1,1,fin);
            fread(&(*mat)[i][j].G,1,1,fin);
            fread(&(*mat)[i][j].R,1,1,fin);
        }
        fseek(fin,pad,SEEK_CUR);                            ///padding-ul nu se memoreaza in matrice
    }
    fclose(fin);
}
void afisare_matrice(char *imagine,RGB **mat,unsigned char *aux,unsigned int w_img,unsigned int h_img)
{
    FILE *fout=fopen(imagine,"wb");
    unsigned int pad=padding(w_img);
    unsigned char p=0;
    for(int i=0; i<54; i++)
        fwrite(&aux[i],1,1,fout);
    for(int i=0; i<h_img; i++)                              ///se incepe afisarea imaginii intr-un fisier
    {
        for(int j=0; j<w_img; j++)
        {
            fwrite(&mat[i][j].B,1,1,fout);
            fwrite(&mat[i][j].G,1,1,fout);
            fwrite(&mat[i][j].R,1,1,fout);
        }
        for(int k=0; k<pad; k++)                            ///se afiseaza padding-ul
            fwrite(&p,1,1,fout);
    }
    fclose(fout);
    for(int i=0; i<h_img; i++)                              ///se dezalocheaza memoria matricii
        free(mat[i]);
    free(mat);
}
void grayscale(RGB ***mat,unsigned int w_img,unsigned int h_img)
{
    unsigned int val;
    for(int i=0; i<h_img; i++)                              ///se realizeaza transformarea imaginii color in una in tonuri de gri
    {
        for(int j=0; j<w_img; j++)
        {
            val=0.299*(*mat)[i][j].B+0.587*(*mat)[i][j].G+0.114*(*mat)[i][j].R;
            (*mat)[i][j].B=(*mat)[i][j].G=(*mat)[i][j].R=val;
        }
    }
}
double med_intensitatii(RGB **mat, unsigned int w_s, unsigned int h_s, int i, int j)
{
    double s=0;
    int aux=j;
    for(int k=0; k<h_s; k++)
    {
        for(int l=0; l<w_s; l++)
        {
            s=s+mat[i][aux].R;
            aux++;
        }
        i++;
        aux=j;
    }
    s=s/(w_s*h_s);
    return s;
}
double deviatia_stan(RGB **mat,unsigned int w_s,unsigned int h_s, int i,int j)
{
    double s=0,n=w_s*h_s,s1;
    int aux=j;
    s1=med_intensitatii(mat,w_s,h_s,i,j);
    for(int k=0; k<h_s; k++)
    {
        for(int l=0; l<w_s; l++)
        {
            s=s+(mat[i][aux].R-s1)*(mat[i][aux].R-s1);
            aux++;
        }
        i++;
        aux=j;
    }
    s=sqrt((1/(n-1)*s));
    return s;
}
void colorare(RGB ***mat,int i,int j,unsigned int w_s,unsigned int h_s,unsigned char r,unsigned char g,unsigned char b)
{
    int aux=j,k;
    for(k=0; k<w_s; k++)                                    ///se coloreaza marginile de sus si de jos
    {
        (*mat)[i][aux].B=(*mat)[i+h_s-1][aux].B=b;
        (*mat)[i][aux].G=(*mat)[i+h_s-1][aux].G=g;
        (*mat)[i][aux].R=(*mat)[i+h_s-1][aux].R=r;
        aux++;
    }
    aux=i;
    for(k=0; k<h_s; k++)                                    ///se coloreaza marginile din stanga si din dreapta
    {
        (*mat)[aux][j].B=(*mat)[aux][j+w_s-1].B=b;
        (*mat)[aux][j].G=(*mat)[aux][j+w_s-1].G=g;
        (*mat)[aux][j].R=(*mat)[aux][j+w_s-1].R=r;
        aux++;
    }
}
typedef struct
{
    int linie;
    int coloana;
    double cor;
    RGB culoare;
} detectie;
void template_matching(char *imagine,char *sablon,double ps,detectie **D, int *poz)
{
    RGB **img,**sab;
    unsigned char *aux1,*aux2;
    unsigned int w_img,w_s,h_img,h_s;
    aux1=header(imagine,&w_img,&h_img);
    aux2=header(sablon,&w_s,&h_s);
    matrice(imagine,&img);
    matrice(sablon,&sab);
    grayscale(&img,w_img,h_img);
    grayscale(&sab,w_s,h_s);
    *D=(detectie*)malloc(w_img*h_img*sizeof(detectie));
    int i,j,k,l,x,y;
    double corr,v_img,v_s,v_img1,v_s1,n;
    n=w_s*h_s;
    v_s=deviatia_stan(sab,w_s,h_s,0,0);                     ///se calculeaza deviatia standard pentru sablon
    v_s1=med_intensitatii(sab,w_s,h_s,0,0);                 ///se calculeaza media intensitatii grayscale a sablonului
    for(i=0; i<h_img; i++)
    {
        for(j=0; j<w_img; j++)
        {
            if(i+h_s<=h_img && j+w_s<=w_img)
            {
                k=i;                                        ///se definesc cordonatele de plecare a ferestrei
                l=j;
                corr=0;
                v_img=deviatia_stan(img,w_s,h_s,k,l);       ///se calculeaza deviatia standard pentru fereasca la care ne aflam
                v_img1=med_intensitatii(img,w_s,h_s,k,l);   ///se calculeaza media grayscale a ferestrei la cere ne aflam
                for(x=0; x<h_s; x++)
                {
                    for(y=0; y<w_s; y++)
                    {
                        corr=corr+(img[k][l].R-v_img1)*(sab[x][y].R-v_s1)/(v_img*v_s);
                        l++;
                    }
                    k++;
                    l=j;
                }
                corr=corr/n;
                if(corr>=ps)                                ///se retin corelatii mai mari decat un prag dat,ps
                {
                    (*D)[(*poz)].coloana=j;
                    (*D)[(*poz)].linie=i;
                    (*D)[(*poz)].cor=corr;
                    (*poz)++;
                }
            }

        }
    }
}
void culoare(detectie **D,int n,unsigned char r,unsigned char g,unsigned char b)
{
    for(int i=0; i<n; i++)                                  ///se indica fiecarii detectii ce culoare are
    {
        (*D)[i].culoare.B=b;
        (*D)[i].culoare.G=g;
        (*D)[i].culoare.R=r;
    }
}
void copie(detectie *D,detectie *d,int n,int *cnt)
{
    for(int i=0; i<n; i++)
    {
        D[(*cnt)].coloana=d[i].coloana;
        D[(*cnt)].cor=d[i].cor;
        D[(*cnt)].culoare=d[i].culoare;
        D[(*cnt)].linie=d[i].linie;
        (*cnt)++;
    }
    free(d);
}
int cmp(const void *a,const void *b)
{
    if(((detectie*)a)->cor>=((detectie*)b)->cor)
        return -1;
    return 1;
}
double arie(int linie1,int linie2,int coloana1,int coloana2)
{
    double l,L;
    if(coloana2>=coloana1)
        l=coloana2-coloana1+1;
    else
        l=coloana1-coloana2+1;
    if(linie2>=linie1)
        L=linie2-linie1+1;
    else
        L=linie1-linie2+1;
    return l*L;
}
void nonmaxime(detectie **d,int *n,unsigned int w_s,unsigned int h_s)
{
    double s;
    double arie_intersectie,arie_di,arie_dj;
    for(int i=0; i<(*n); i++)
        for(int j=i+1; j<(*n); j++)
        {
            ///se verifica daca doua detectii se intersecteaza
            if((*d)[j].linie>=(*d)[i].linie && (*d)[j].linie<=(*d)[i].linie+h_s-1 && (*d)[j].coloana>=(*d)[i].coloana && (*d)[j].coloana<=(*d)[i].coloana+w_s-1)
            {
                arie_intersectie=arie((*d)[j].linie,(*d)[i].linie+h_s-1,(*d)[j].coloana,(*d)[i].coloana+w_s-1);
                arie_di=arie((*d)[i].linie,(*d)[i].linie+h_s-1,(*d)[i].coloana,(*d)[i].coloana+w_s-1);
                arie_dj=arie((*d)[j].linie,(*d)[j].linie+h_s-1,(*d)[j].coloana,(*d)[j].coloana+w_s-1);
                s=arie_intersectie/(arie_di+arie_dj-arie_intersectie);
                if(s>0.20)
                {
                    for(int k=j; k<(*n)-1; k++)
                    {
                        (*d)[k]=(*d)[k+1];
                    }
                    (*n)--;
                    j--;
                }
            }
            else if((*d)[i].linie>=(*d)[j].linie && (*d)[i].linie<=(*d)[j].linie+h_s-1 && (*d)[i].coloana>=(*d)[j].coloana && (*d)[i].coloana<=(*d)[j].coloana+w_s-1)
            {
                arie_intersectie=arie((*d)[i].linie,(*d)[j].linie+h_s-1,(*d)[i].coloana,(*d)[j].coloana+w_s-1);
                arie_di=arie((*d)[i].linie,(*d)[i].linie+h_s-1,(*d)[i].coloana,(*d)[i].coloana+w_s-1);
                arie_dj=arie((*d)[j].linie,(*d)[j].linie+h_s-1,(*d)[j].coloana,(*d)[j].coloana+w_s-1);
                s=arie_intersectie/(arie_di+arie_dj-arie_intersectie);
                if(s>0.20)
                {
                    for(int k=j; k<(*n)-1; k++)
                    {
                        (*d)[k]=(*d)[k+1];
                    }
                    (*n)--;
                    j--;
                }
            }
            else if((*d)[j].linie>=(*d)[i].linie && (*d)[j].linie<=(*d)[i].linie+h_s-1 && (*d)[j].coloana+w_s-1>=(*d)[i].coloana && (*d)[j].coloana+w_s-1<=(*d)[i].coloana+w_s-1)
            {
                arie_intersectie=((*d)[j].linie,(*d)[i].linie+h_s-1,(*d)[i].coloana,(*d)[j].coloana+w_s-1);
                arie_di=arie((*d)[i].linie,(*d)[i].linie+h_s-1,(*d)[i].coloana,(*d)[i].coloana+w_s-1);
                arie_dj=arie((*d)[j].linie,(*d)[j].linie+h_s-1,(*d)[j].coloana,(*d)[j].coloana+w_s-1);
                s=arie_intersectie/(arie_di+arie_dj-arie_intersectie);
                if(s>0.20)
                {
                    for(int k=j; k<(*n)-1; k++)
                    {
                        (*d)[k]=(*d)[k+1];
                    }
                    (*n)--;
                    j--;
                }
            }
            else if((*d)[i].linie>=(*d)[j].linie && (*d)[i].linie<=(*d)[j].linie+h_s-1 && (*d)[i].coloana+w_s-1>=(*d)[j].coloana && (*d)[i].coloana+w_s-1<=(*d)[j].coloana+w_s-1)
            {
                arie_intersectie=arie((*d)[i].linie,(*d)[j].linie+h_s-1,(*d)[j].coloana,(*d)[i].coloana+w_s-1);
                arie_di=arie((*d)[i].linie,(*d)[i].linie+h_s-1,(*d)[i].coloana,(*d)[i].coloana+w_s-1);
                arie_dj=arie((*d)[j].linie,(*d)[j].linie+h_s-1,(*d)[j].coloana,(*d)[j].coloana+w_s-1);
                s=arie_intersectie/(arie_di+arie_dj-arie_intersectie);
                if(s>0.20)
                {
                    for(int k=j; k<(*n)-1; k++)
                    {
                        (*d)[k]=(*d)[k+1];
                    }
                    (*n)--;
                    j--;
                }
            }

        }
}
int main()
{
    char *nume_imagine,*img_criptata,*imagine1,*fisier,*imagine2;
    nume_imagine=(char*)malloc(101*sizeof(char));
    img_criptata=(char*)malloc(101*sizeof(char));
    imagine1=(char*)malloc(101*sizeof(char));
    imagine2=(char*)malloc(101*sizeof(char));
    fisier=(char*)malloc(101*sizeof(char));
    printf("       CRIPTARE\n");
    printf("Numele fisierului unde se afla imaginea este: ");
    fgets(nume_imagine, 101, stdin);
    nume_imagine[strlen(nume_imagine) - 1] = '\0';
    printf("\nNumele fisierului in care se salveaza imaginea criptata este: ");
    fgets(imagine1, 101, stdin);
    imagine1[strlen(imagine1) - 1] = '\0';
    printf("\nNumele fisierului care contine cheia secreta este: ");
    fgets(fisier, 101, stdin);
    fisier[strlen(fisier) - 1] = '\0';
    criptare(nume_imagine,imagine1,fisier);
    printf("\n\n       DECRIPTAREA\n");
    printf("Numele fisierului unde se afla imaginea criptata este: ");
    fgets(img_criptata, 101, stdin);
    img_criptata[strlen(img_criptata) - 1] = '\0';
    printf("\nNumele fisierului in care se salveaza imaginea decriptata este: ");
    fgets(imagine2, 101, stdin);
    imagine2[strlen(imagine2) - 1] = '\0';
    printf("\nNumele fisierului care contine cheia secreta este: ");
    fgets(fisier, 101, stdin);
    fisier[strlen(fisier) - 1] = '\0';
    decriptare(imagine2,img_criptata,fisier);
    printf("\n\n       CHI-PATRAT\n");
    printf("Valorile testului chi-patrat a imaginii necriptate sunt: \n\n");
    chi_patrat(nume_imagine);
    printf("\nValorile testului chi- patrat a imaginii criptate sunt: \n\n");
    chi_patrat(img_criptata);
    free(nume_imagine);
    free(img_criptata);
    free(imagine1);
    free(imagine2);
    free(fisier);
    printf("\n\n       RECUNOASTEREA PATTERN-URILOR INTR-O IMAGINE\n");
    char *imagine,*sablon,*imagine_finala;
    imagine=(char*)malloc(101*sizeof(char));
    sablon=(char*)malloc(101*sizeof(char));
    imagine_finala=(char*)malloc(101*sizeof(char));
    printf("Numele fisierului unde se afla imaginea este: ");
    fgets(imagine, 101, stdin);
    imagine[strlen(imagine) - 1] = '\0';
    printf("\nCitirea sabloanelor: ");
    printf("\nNumele fisierului unde se afla primul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    RGB **mat;
    unsigned int w_s,h_s,w_img,h_img;
    int poz0,poz1,poz2,poz3,poz4,poz5,poz6,poz7,poz8,poz9,n=0;
    poz0=poz1=poz2=poz3=poz4=poz5=poz6=poz7=poz8=poz9=0;
    unsigned char *aux1,*aux2;
    aux1=header(sablon,&w_s,&h_s);
    aux2=header(imagine,&w_img,&h_img);
    matrice(imagine,&mat);
    detectie *D,*D0,*D1,*D2,*D3,*D4,*D5,*D6,*D7,*D8,*D9;
    template_matching(imagine,sablon,0.50,&D0,&poz0);
    culoare(&D0,poz0,255,0,0);
    n=n+poz0;
    printf("Numele fisierului unde se afla urmatorul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    template_matching(imagine,sablon,0.50,&D1,&poz1);
    culoare(&D1,poz1,255,255,0);
    n=n+poz1;
    printf("Numele fisierului unde se afla urmatorul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    template_matching(imagine,sablon,0.50,&D2,&poz2);
    culoare(&D2,poz2,0,255,0);
    n=n+poz2;
    printf("Numele fisierului unde se afla urmatorul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    template_matching(imagine,sablon,0.50,&D3,&poz3);
    culoare(&D3,poz3,0,255,255);
    n=n+poz3;
    printf("Numele fisierului unde se afla urmatorul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    template_matching(imagine,sablon,0.50,&D4,&poz4);
    culoare(&D4,poz4,255,0,255);
    n=n+poz4;
    printf("Numele fisierului unde se afla urmatorul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    template_matching(imagine,sablon,0.50,&D5,&poz5);
    culoare(&D5,poz5,0,0,255);
    n=n+poz5;
    printf("Numele fisierului unde se afla urmatorul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    template_matching(imagine,sablon,0.50,&D6,&poz6);
    culoare(&D6,poz6,192,192,192);
    n=n+poz6;
    printf("Numele fisierului unde se afla urmatorul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    template_matching(imagine,sablon,0.50,&D7,&poz7);
    culoare(&D7,poz7,255,140,0);
    n=n+poz7;
    printf("Numele fisierului unde se afla urmatorul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    template_matching(imagine,sablon,0.50,&D8,&poz8);
    culoare(&D8,poz8,128,0,128);
    n=n+poz8;
    printf("Numele fisierului unde se afla urmatorul sablon este: ");
    fgets(sablon, 101, stdin);
    sablon[strlen(sablon) - 1] = '\0';
    template_matching(imagine,sablon,0.50,&D9,&poz9);
    culoare(&D9,poz9,128,0,0);
    n=n+poz9;
    D=(detectie*)malloc(n*sizeof(detectie));                ///se alipesc toti vectorii de detectii intr-un singur vector
    int cnt=0;
    copie(D,D0,poz0,&cnt);
    copie(D,D1,poz1,&cnt);
    copie(D,D2,poz2,&cnt);
    copie(D,D3,poz3,&cnt);
    copie(D,D4,poz4,&cnt);
    copie(D,D5,poz5,&cnt);
    copie(D,D6,poz6,&cnt);
    copie(D,D7,poz7,&cnt);
    copie(D,D8,poz8,&cnt);
    copie(D,D9,poz9,&cnt);
    qsort(D,cnt,sizeof(detectie),cmp);                      ///se sorteaza descrescator vectorul
    nonmaxime(&D,&cnt,w_s,h_s);                             ///se elimina detectiile care se suprapun spatial
    for(int i=0; i<cnt; i++)                                ///se deseneaza fiecare detectie
        colorare(&mat,D[i].linie,D[i].coloana,w_s,h_s,D[i].culoare.R,D[i].culoare.G,D[i].culoare.B);
    printf("\nNumele fisierului unde se afla imaginea nou este: ");
    fgets(imagine_finala, 101, stdin);
    imagine_finala[strlen(imagine_finala) - 1] = '\0';
    afisare_matrice(imagine_finala,mat,aux2,w_img,h_img);
    free(D);
    free(aux1);
    free(aux2);
    free(imagine);
    free(imagine_finala);
    free(sablon);
    return 0;
}
