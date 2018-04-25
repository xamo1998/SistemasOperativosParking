#ifndef _HPUX_SOURCE
  #define _HPUX_SOURCE
#endif

#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "parking.h"
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/ipc.h>

#define ACERA 80
#define PRIMERAJUSTEMENSAJES 1000
#define SIGUIENTEAJUSTEMENSAJES 26000
#define MEJORAJUSTEMENSAJES 51000
#define PEORAJUSTEMENSAJES 66000
#define MENSAJETIPO 99998
#define TIPOORDEN 99999
#define NUMSEMAFORO 1 + (ACERA*4)
#define NUMFLAGS 1
#define MEMSIZE (ACERA*4)+(NUMFLAGS)+4
#define DIMVECTOR 100


#define HUECOPRIMERPOS PARKING_getTamaNoMemoriaCompartida()+1
#define HUECOSIGUIENTEPOS  HUECOPRIMERPOS+ACERA+1
#define HUECOMEJORPOS HUECOSIGUIENTEPOS+ACERA+1
#define HUECOPEORPOS HUECOMEJORPOS+ACERA+1
#define SIGUIENTEPOSICION HUECOPEORPOS+ACERA+1



#define FLAGSEMAFOROMEMORIA PARKING_getNSemAforos()

#define POSSEMAFOROCARRETERAMEJOR FLAGSEMAFOROMEMORIA +1
#define POSSEMAFOROCARRETERAPRIMER POSSEMAFOROCARRETERAMEJOR + ACERA
#define POSSEMAFOROCARRETERAPEOR POSSEMAFOROCARRETERAPRIMER + ACERA
#define POSSEMAFOROCARRETERASIGUIENTE POSSEMAFOROCARRETERAPEOR + ACERA


#define clear() printf("\033[H\033[J")
//Variable globales
typedef struct Data{
    int idSemaforo, idBuzon, idMemoria, *pidChofers, repartidorPID, ipcsRemoved, numChoferes, parentPID;
    int prioridadAparcar, prioridadDesaparcar;
    }Data;
Data data;
union semun 
        {int             val;
         struct semid_ds *buf;
         ushort_t        *array;
         };

//Prototipos funciones//
void compruebaArg(int, char *[], int *);
void sigintHandler(int sig);
void sigalrmHandler(int sig);
TIPO_FUNCION_LLEGADA mejorAjuste(HCoche hc);
TIPO_FUNCION_LLEGADA peorAjuste(HCoche hc);
TIPO_FUNCION_LLEGADA siguienteAjuste(HCoche hc);
TIPO_FUNCION_LLEGADA primerAjuste(HCoche hc);
TIPO_FUNCION_APARCAR_COMMIT aparcarCommit(HCoche hc);
TIPO_FUNCION_PERMISO_AVANCE permisoAvanzar(HCoche hc);
TIPO_FUNCION_PERMISO_AVANCE_COMMIT permisoAvanzarCommit(HCoche hc);

void updateCarreteraVertical(int algoritmo, int posInicial, int posFinal,int s, int posVertical, int longitud);
void updateCarreteraHorizontal(int algoritmo, int posInicial, int longitud);
void updateAcera(int algoritmo, int posInicial, int longitud);
void creaManejadoras();
int creaAlarm();
void creaIPCS();
int creaChofers();
int creaRepartidor();
void eliminaIPCS(); 
void waitSem(int nSemaforo);
void signalSem(int nSemaforo);
void inicializaMemoria();


int main(int argc, char *argv[]){
    int debug=0;
    data.numChoferes=atoi(argv[2]);
    data.parentPID=getpid();
    TIPO_FUNCION_LLEGADA f_llegadas[]={(TIPO_FUNCION_LLEGADA)primerAjuste,
                                       (TIPO_FUNCION_LLEGADA)siguienteAjuste,
                                       (TIPO_FUNCION_LLEGADA)mejorAjuste,
                                       (TIPO_FUNCION_LLEGADA)peorAjuste};
    compruebaArg(argc, argv, &debug);
    creaManejadoras();
    creaIPCS();
    inicializaMemoria();
    PARKING_inicio(atoi(argv[1]), f_llegadas, data.idSemaforo, data.idBuzon, data.idMemoria, debug);
    creaRepartidor();
    creaChofers();
    creaAlarm();
    PARKING_simulaciOn();
    pause();
    return 0;
}

void inicializaMemoria(){
    int i;
    char *dataMem;
    dataMem = shmat(data.idMemoria, (void *)0, 0);
    if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
    
    for(i=PARKING_getTamaNoMemoriaCompartida();i<SIGUIENTEPOSICION;i++)
        dataMem[i]=0;
    dataMem[SIGUIENTEPOSICION]=-3;
    if (shmdt(dataMem) == -1) {
        perror("Parking:shmdt");
    }

}

TIPO_FUNCION_LLEGADA mejorAjuste(HCoche hc){
    int matrizHuecos[ACERA][2];
    int i,j, longitudCoche=PARKING_getLongitud(hc), numHuecos=0;
    int contador=0, huecoMin=0, posFinal=0, posInicial;
    char *dataMem;
    struct sembuf sops[1];
    
    waitSem(FLAGSEMAFOROMEMORIA);
    
    dataMem = shmat(data.idMemoria, (void *)0, 0);
 
    if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
    for(i=0;i<ACERA;i++)
        for(j=0;j<2;j++)
            matrizHuecos[i][j]=0;

    for(i=0;i<ACERA;i++){
         if(dataMem[HUECOMEJORPOS+i]!=1){
            contador++;
            if(contador==longitudCoche){
                matrizHuecos[numHuecos][0]=contador; //Tamaño
                matrizHuecos[numHuecos][1]=i-longitudCoche+1;
                numHuecos++;
            }else if(contador>longitudCoche){ //Me sobran huecos
                matrizHuecos[numHuecos-1][0]=contador;//Tamaño
                matrizHuecos[numHuecos-1][1]=matrizHuecos[numHuecos-1][1];
            }
         }else{
            contador=0;
         }
            
               
            
     }
     
     huecoMin=100;
     for(i=0;i<ACERA;i++){
        if(huecoMin>matrizHuecos[i][0] && matrizHuecos[i][0]!=0){
            huecoMin=matrizHuecos[i][0];
            posInicial=matrizHuecos[i][1];
        }
     }
        
     if(huecoMin>=longitudCoche && huecoMin!=100){
        for(i=posInicial;i<posInicial+longitudCoche;i++)
            dataMem[HUECOMEJORPOS+i]=1;
        signalSem(FLAGSEMAFOROMEMORIA);
        if(shmdt(dataMem)==-1){
           perror("Parking:shmdt");
         }
        return (TIPO_FUNCION_LLEGADA) posInicial;
     }
         
  
     if(shmdt(dataMem)==-1){
           perror("Parking:shmdt");
         }
     signalSem(FLAGSEMAFOROMEMORIA);
     return (TIPO_FUNCION_LLEGADA) -1;
}


TIPO_FUNCION_LLEGADA peorAjuste(HCoche hc){
    int matrizHuecos[ACERA][2];
    int i,j, longitudCoche=PARKING_getLongitud(hc), numHuecos=0;
    int contador=0, huecoMax=0, posFinal=0, posInicial;
    char *dataMem;
    struct sembuf sops[1];
    
    waitSem(FLAGSEMAFOROMEMORIA);
    
    dataMem = shmat(data.idMemoria, (void *)0, 0);
 
    if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
    for(i=0;i<ACERA;i++)
        for(j=0;j<2;j++)
            matrizHuecos[i][j]=0;

    for(i=0;i<ACERA;i++){
         if(dataMem[HUECOPEORPOS+i]!=1){
            contador++;
            if(contador==longitudCoche){
                matrizHuecos[numHuecos][0]=contador; //Tamaño
                matrizHuecos[numHuecos][1]=i-longitudCoche+1;
                numHuecos++;
            }else if(contador>longitudCoche){ //Me sobran huecos
                matrizHuecos[numHuecos][0]=contador;//Tamaño
                matrizHuecos[numHuecos][1]=matrizHuecos[numHuecos-1][1];
                numHuecos++;
            }
         }else{
            contador=0;
         }
       
                     
     }
     for(i=0;i<ACERA;i++){
        if(huecoMax<matrizHuecos[i][0]){
            huecoMax=matrizHuecos[i][0];
            posInicial=matrizHuecos[i][1];
        }
     }
     if(huecoMax>=longitudCoche){
        for(i=posInicial;i<posInicial+longitudCoche;i++)
            dataMem[HUECOPEORPOS+i]=1;
        if(shmdt(dataMem)==-1){
           perror("Parking:shmdt");
         }
        signalSem(FLAGSEMAFOROMEMORIA);
        return (TIPO_FUNCION_LLEGADA) posInicial;
     }
         
  
     if(shmdt(dataMem)==-1){
           perror("Parking:shmdt");
         }
     signalSem(FLAGSEMAFOROMEMORIA);
     return (TIPO_FUNCION_LLEGADA) -1;


    }

TIPO_FUNCION_LLEGADA siguienteAjuste(HCoche hc){
   	int lastPosition=-5,flag=0;
    char *dataMem;
    waitSem(FLAGSEMAFOROMEMORIA);
    dataMem = shmat(data.idMemoria, (void *)0, 0);
    if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
    int i,j, longitudCoche=PARKING_getLongitud(hc), contador=0;
    lastPosition=dataMem[SIGUIENTEPOSICION];
    if(lastPosition==-3){
    	
        lastPosition=0;//Para el bucle.
    }
    if(dataMem[HUECOSIGUIENTEPOS+lastPosition]!=1){
    	for(i=lastPosition;i>=0;i--){
    		if(dataMem[HUECOSIGUIENTEPOS+i]==1 && flag==0){
    			lastPosition=i;
    			flag=1;
    		}
    		if(i==0 && flag==0)
    			lastPosition=0;
    	}
    }

    for(i=lastPosition; i<ACERA; i++){
        if(dataMem[HUECOSIGUIENTEPOS+i]==0){
            
            contador++;
            if(contador==longitudCoche){
                for(j=i-longitudCoche+1; j<=i;j++){
                        dataMem[HUECOSIGUIENTEPOS+j]=1;
                    }
                    dataMem[SIGUIENTEPOSICION]=i;
                if(shmdt(dataMem)==-1){
                    perror("Parking:shmdt");
                }
                signalSem(FLAGSEMAFOROMEMORIA);
                return (TIPO_FUNCION_LLEGADA)(i-longitudCoche)+1;
            }

        }else{
            contador=0;
        }
    }
 
    contador=0;
    for(i=0; i< lastPosition; i++){
        if(dataMem[HUECOSIGUIENTEPOS+i]!=1){
            contador++;
            if(contador==longitudCoche){
                for(j=i-longitudCoche+1; j<=i;j++){
                        dataMem[HUECOSIGUIENTEPOS+j]=1;
                    }
                    dataMem[SIGUIENTEPOSICION]=i;
                if(shmdt(dataMem)==-1){
                   perror("Parking:shmdt");
                 }
                signalSem(FLAGSEMAFOROMEMORIA);
                return (TIPO_FUNCION_LLEGADA)(i-longitudCoche)+1;
            }

        }else{
            contador=0;
        }
    }
    if(shmdt(dataMem)==-1){
           perror("Parking:shmdt");
         }
    signalSem(FLAGSEMAFOROMEMORIA);
    return (TIPO_FUNCION_LLEGADA) -1;


}


TIPO_FUNCION_LLEGADA primerAjuste(HCoche hc){
    char *dataMem;
    waitSem(FLAGSEMAFOROMEMORIA);
    dataMem = shmat(data.idMemoria, (void *)0, 0);
    if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
    int i,j, longitudCoche=PARKING_getLongitud(hc), contador=0;

        for(i=0; i<ACERA; i++){
            if(dataMem[HUECOPRIMERPOS+i]!=1){
                contador++;
                if(contador==longitudCoche){
                    
                    for(j=i-longitudCoche+1; j<=i;j++){
                        dataMem[HUECOPRIMERPOS+j]=1;
                    }
                    if (shmdt(dataMem) == -1) {
                        perror("Parking:shmdt");
                    }

                    signalSem(FLAGSEMAFOROMEMORIA);
                    return (TIPO_FUNCION_LLEGADA) (i-longitudCoche)+1;
                }

            }else{
                contador=0;
            }

        }
        if (shmdt(dataMem) == -1) {
            perror("Parking:shmdt");
        }
        signalSem(FLAGSEMAFOROMEMORIA);

        return (TIPO_FUNCION_LLEGADA) -1;
        


}

TIPO_FUNCION_APARCAR_COMMIT aparcarCommit(HCoche hc){
    char *dataMem;
    int recibo;
    struct PARKING_mensajeBiblioteca mensaje;
    waitSem(FLAGSEMAFOROMEMORIA);
    mensaje.subtipo=hc+1;
    mensaje.hCoche=hc;
    switch(PARKING_getAlgoritmo(hc)){
    	case 0:
    		mensaje.tipo=PARKING_getNUmero(hc)+PRIMERAJUSTEMENSAJES + 1;
    		recibo=msgsnd(data.idBuzon,(struct msgbuf *)&mensaje, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), 1);
            if(recibo==-1)
                perror("parking:msgSend");
            break;
        case 1:
        	mensaje.tipo=PARKING_getNUmero(hc)+SIGUIENTEAJUSTEMENSAJES + 1;
    		recibo=msgsnd(data.idBuzon,(struct msgbuf *)&mensaje, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), 1);
            if(recibo==-1)
                perror("parking:msgSend");
            break;
        case 2:
        	mensaje.tipo=PARKING_getNUmero(hc)+MEJORAJUSTEMENSAJES + 1;
    		recibo=msgsnd(data.idBuzon,(struct msgbuf *)&mensaje, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), 1);
            if(recibo==-1)
                perror("parking:msgSend");
            break;
        case 3:
        	mensaje.tipo=PARKING_getNUmero(hc)+PEORAJUSTEMENSAJES + 1;
    		recibo=msgsnd(data.idBuzon,(struct msgbuf *)&mensaje, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), 1);
            if(recibo==-1)
                perror("parking:msgSend");
            break;
    }
    
    signalSem(FLAGSEMAFOROMEMORIA);
}

TIPO_FUNCION_PERMISO_AVANCE permisoAvanzar(HCoche hc){
    int i;    
    switch(PARKING_getAlgoritmo(hc)){
        case 0: //Primer Ajuste...
            if(PARKING_getX(hc)!= PARKING_getX2(hc)){
                if(PARKING_getX2(hc)>=0){
                    waitSem(POSSEMAFOROCARRETERAPRIMER+PARKING_getX2(hc));
                }
                
            }
            if(PARKING_getY2(hc)==2 && PARKING_getY(hc)==1){//Si estoy en la linea y voy a la carretera...
                for(i=PARKING_getX(hc)+PARKING_getLongitud(hc)-1; i>=PARKING_getX(hc);i--){
                    waitSem(POSSEMAFOROCARRETERAPRIMER+i);//Wait sobre la ultima posicion por la derecha...
                }

            }
            return (TIPO_FUNCION_PERMISO_AVANCE)1; //Puede moverse
            break; //End of primer ajuste.

        case 1:
            if(PARKING_getX(hc)!= PARKING_getX2(hc)){
                if(PARKING_getX2(hc)>=0){
                    waitSem(POSSEMAFOROCARRETERASIGUIENTE+PARKING_getX2(hc));
                }
                
            }
            if(PARKING_getY2(hc)==2 && PARKING_getY(hc)==1){//Si estoy en la linea y voy a la carretera...
                for(i=PARKING_getX(hc)+PARKING_getLongitud(hc)-1; i>=PARKING_getX(hc);i--){
                    waitSem(POSSEMAFOROCARRETERASIGUIENTE+i);//Wait sobre la ultima posicion por la derecha...
                }

            }
            return (TIPO_FUNCION_PERMISO_AVANCE)1; //Puede moverse
            break; //End of primer ajuste.

        case 2: //Mejor ajuste
            if(PARKING_getX(hc)!= PARKING_getX2(hc)){
                if(PARKING_getX2(hc)>=0){
                    waitSem(POSSEMAFOROCARRETERAMEJOR+PARKING_getX2(hc));
                }
                
            }
            if(PARKING_getY2(hc)==2 && PARKING_getY(hc)==1){//Si estoy en la linea y voy a la carretera...
                for(i=PARKING_getX(hc)+PARKING_getLongitud(hc)-1; i>=PARKING_getX(hc);i--){
                    waitSem(POSSEMAFOROCARRETERAMEJOR+i);//Wait sobre la ultima posicion por la derecha...
                }

            }
            return (TIPO_FUNCION_PERMISO_AVANCE)1; //Puede moverse
            break; //End of mejor ajuste.
        case 3://Peor ajuste
            if(PARKING_getX(hc)!= PARKING_getX2(hc)){
                if(PARKING_getX2(hc)>=0){
                    waitSem(POSSEMAFOROCARRETERAPEOR+PARKING_getX2(hc));
                }
                
            }

            if(PARKING_getY2(hc)==2 && PARKING_getY(hc)==1){//Si estoy en la linea y voy a la carretera...
                for(i=PARKING_getX(hc)+PARKING_getLongitud(hc)-1; i>=PARKING_getX(hc);i--){
                    waitSem(POSSEMAFOROCARRETERAPEOR+i);//Wait sobre la ultima posicion por la derecha...
                }

            }

            return (TIPO_FUNCION_PERMISO_AVANCE)1; //Puede moverse
            break; //End of peor ajuste.
    }
}

TIPO_FUNCION_PERMISO_AVANCE_COMMIT permisoAvanzarCommit(HCoche hc){
    int i;
    char *dataMem;
    switch(PARKING_getAlgoritmo(hc)){
        case 0:

            if(PARKING_getX(hc)==PARKING_getX2(hc)){//Ha sido un movimiento vertical.
                updateCarreteraVertical(PARKING_getAlgoritmo(hc), PARKING_getX(hc),PARKING_getX2(hc), PARKING_getY(hc),PARKING_getY2(hc), PARKING_getLongitud(hc));
            }else{
                updateCarreteraHorizontal(PARKING_getAlgoritmo(hc), PARKING_getX2(hc), PARKING_getLongitud(hc));
            }


                if(PARKING_getY2(hc)==1 && PARKING_getY(hc)==2){
                    updateAcera(PARKING_getAlgoritmo(hc), PARKING_getX(hc), PARKING_getLongitud(hc));
            }
                                      
            break;
        case 1:
            if(PARKING_getX(hc)==PARKING_getX2(hc)){//Ha sido un movimiento vertical.
                updateCarreteraVertical(PARKING_getAlgoritmo(hc), PARKING_getX(hc),PARKING_getX2(hc), PARKING_getY(hc),PARKING_getY2(hc), PARKING_getLongitud(hc));
            }else{
                updateCarreteraHorizontal(PARKING_getAlgoritmo(hc), PARKING_getX2(hc), PARKING_getLongitud(hc));
            }


                if(PARKING_getY2(hc)==1 && PARKING_getY(hc)==2){
                    updateAcera(PARKING_getAlgoritmo(hc), PARKING_getX(hc), PARKING_getLongitud(hc));
            }
            
            break;

        case 2:
            if(PARKING_getX(hc)==PARKING_getX2(hc)){//Ha sido un movimiento vertical.
                updateCarreteraVertical(PARKING_getAlgoritmo(hc), PARKING_getX(hc),PARKING_getX2(hc), PARKING_getY(hc),PARKING_getY2(hc), PARKING_getLongitud(hc));
            }else{
                updateCarreteraHorizontal(PARKING_getAlgoritmo(hc), PARKING_getX2(hc), PARKING_getLongitud(hc));
            }


                if(PARKING_getY2(hc)==1 && PARKING_getY(hc)==2){
                    updateAcera(PARKING_getAlgoritmo(hc), PARKING_getX(hc), PARKING_getLongitud(hc));
            }
            
                
            break;


        case 3:
            if(PARKING_getX(hc)==PARKING_getX2(hc)){//Ha sido un movimiento vertical.
                updateCarreteraVertical(PARKING_getAlgoritmo(hc), PARKING_getX(hc),PARKING_getX2(hc), PARKING_getY(hc),PARKING_getY2(hc), PARKING_getLongitud(hc));
            }else{
                updateCarreteraHorizontal(PARKING_getAlgoritmo(hc), PARKING_getX2(hc), PARKING_getLongitud(hc));
            }


                if(PARKING_getY2(hc)==1 && PARKING_getY(hc)==2){
                    updateAcera(PARKING_getAlgoritmo(hc), PARKING_getX(hc), PARKING_getLongitud(hc));
            }
            

            break;
    }

}

void updateCarreteraVertical(int algoritmo, int posActual, int posPrevia, int posVerticalActual,int posVerticalPrevia, int longitud){
    int i;
    switch(algoritmo){
        case 0:
            if(posVerticalPrevia==2)
                for(i=posActual; i<posActual+longitud; i++)
                    signalSem(POSSEMAFOROCARRETERAPRIMER+i);
            break;
        case 1:
            if(posVerticalPrevia==2)
                for(i=posActual; i<posActual+longitud; i++)
                    signalSem(POSSEMAFOROCARRETERASIGUIENTE+i);
            break;
        case 2:
            if(posVerticalPrevia==2)
                for(i=posActual; i<posActual+longitud; i++)
                    signalSem(POSSEMAFOROCARRETERAMEJOR+i);
            break;
        case 3:
            if(posVerticalPrevia==2)
                for(i=posActual; i<posActual+longitud; i++)
                    signalSem(POSSEMAFOROCARRETERAPEOR+i);
            break;
        
    }
}


void updateCarreteraHorizontal(int algoritmo, int posInicial, int longitud){
    int i;  
    switch(algoritmo){
        case 0:
            if(posInicial<0){ //Estamos fuera de la pantalla    
                signalSem(POSSEMAFOROCARRETERAPRIMER+posInicial+longitud-1);//Ponemos libre la primera posicion de la carretera.
            }else if(posInicial==0){
                signalSem(POSSEMAFOROCARRETERAPRIMER+longitud-1);
            }else if(posInicial+longitud-1<ACERA){
                signalSem(POSSEMAFOROCARRETERAPRIMER+posInicial+longitud-1);
                }
            break;
        case 1:
            if(posInicial<0){ //Estamos fuera de la pantalla    
                signalSem(POSSEMAFOROCARRETERASIGUIENTE+posInicial+longitud-1);//Ponemos libre la primera posicion de la carretera.
            }else if(posInicial==0){
                signalSem(POSSEMAFOROCARRETERASIGUIENTE+longitud-1);
            }else if(posInicial+longitud-1<ACERA){
                signalSem(POSSEMAFOROCARRETERASIGUIENTE+posInicial+longitud-1);
                }
            break;

        case 2:
            if(posInicial<0){ //Estamos fuera de la pantalla    
                signalSem(POSSEMAFOROCARRETERAMEJOR+posInicial+longitud-1);//Ponemos libre la primera posicion de la carretera.
            }else if(posInicial==0){
                signalSem(POSSEMAFOROCARRETERAMEJOR+longitud-1);
            }else if(posInicial+longitud-1<ACERA){
                signalSem(POSSEMAFOROCARRETERAMEJOR+posInicial+longitud-1);
                }    
            break;

        case 3:
            if(posInicial<0){ //Estamos fuera de la pantalla    
                signalSem(POSSEMAFOROCARRETERAPEOR+posInicial+longitud-1);//Ponemos libre la primera posicion de la carretera.
            }else if(posInicial==0){
                signalSem(POSSEMAFOROCARRETERAPEOR+longitud-1);
            }else if(posInicial+longitud-1<ACERA){
                signalSem(POSSEMAFOROCARRETERAPEOR+posInicial+longitud-1);
                }   
            break;

    }
}
void updateAcera(int algoritmo, int posInicial, int longitud){
    int i;
    struct sembuf sops[1];
    char *dataMem;
    waitSem(FLAGSEMAFOROMEMORIA);
    dataMem = shmat(data.idMemoria, (void *)0, 0);
    if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
    switch(algoritmo){
        case 0: //Primer algoritmo.
            for(i=posInicial; i< (posInicial+longitud); i++){
                dataMem[HUECOPRIMERPOS+i]=0;//data.huecoMejor[0][i]=0;
            }           
            if (shmdt(dataMem) == -1) {
                perror("Parking:shmdt");
            }
            signalSem(FLAGSEMAFOROMEMORIA);
            break;
        case 1:

            for(i=posInicial; i< (posInicial+longitud); i++){
                dataMem[HUECOSIGUIENTEPOS+i]=0;//data.huecoMejor[0][i]=0;
            }
            
            if (shmdt(dataMem) == -1) {
                perror("Parking:shmdt");
            }
            signalSem(FLAGSEMAFOROMEMORIA);
            break;

        case 2:
            for(i=posInicial; i< (posInicial+longitud); i++){
                dataMem[HUECOMEJORPOS+i]=0;//data.huecoMejor[0][i]=0;
            }
            
            if (shmdt(dataMem) == -1) {
                perror("Parking:shmdt");
            }
            signalSem(FLAGSEMAFOROMEMORIA);
            break;
        case 3:
            for(i=posInicial; i< (posInicial+longitud); i++){
                dataMem[HUECOPEORPOS+i]=0;//data.huecoMejor[0][i]=0;
            }
            
            if (shmdt(dataMem) == -1) {
                perror("Parking:shmdt");
            }
            signalSem(FLAGSEMAFOROMEMORIA);
            break;       
    }
}


int creaRepartidor(){
    int recibo;
    struct PARKING_mensajeBiblioteca mensaje, mensajeReescrito, prioridades[DIMVECTOR];
    
    switch(fork()){
        case -1:
            perror("Parking:forkRepartidor");
            exit(1);
        case 0:
        data.repartidorPID=getpid();
        if(data.prioridadDesaparcar==1){
        	while(1){
        		recibo=msgrcv(data.idBuzon,&mensaje, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), PARKING_MSG, 0);
                if(recibo==-1){
                    perror("parking:reciboBuzon");
                    exit(1);
                }
                if(mensaje.subtipo==1) //Aparcar
                	mensajeReescrito.tipo=2;
                else
                	mensajeReescrito.tipo=1;
                mensajeReescrito.subtipo=mensaje.subtipo;
                mensajeReescrito.hCoche=mensaje.hCoche;
                recibo=msgsnd(data.idBuzon,(struct msgbuf *)&mensajeReescrito, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), 0);
                if(recibo==-1){
                    perror("parking:msgSend");
                    exit(1);
                }
        	}

        }else if(data.prioridadAparcar==1){
        		while(1){
        		recibo=msgrcv(data.idBuzon,&mensaje, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), PARKING_MSG, 0);
                if(recibo==-1){
                    perror("parking:reciboBuzon");
                    exit(1);
                }
                if(mensaje.subtipo==1) //Aparcar
                	mensajeReescrito.tipo=1;
                else
                	mensajeReescrito.tipo=2;
                mensajeReescrito.subtipo=mensaje.subtipo;
                mensajeReescrito.hCoche=mensaje.hCoche;
                recibo=msgsnd(data.idBuzon,(struct msgbuf *)&mensajeReescrito, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), 0);
                if(recibo==-1){
                    perror("parking:msgSend");
                    exit(1);
                }
        	}
        }else{
        	while(1){
                recibo=msgrcv(data.idBuzon,&mensaje, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), PARKING_MSG, 0);
                if(recibo==-1){
                    perror("parking:reciboBuzon");
                    exit(1);
                }

                //Reescribir mensaje.
                mensajeReescrito.tipo=MENSAJETIPO;
                mensajeReescrito.subtipo=mensaje.subtipo;
                mensajeReescrito.hCoche=mensaje.hCoche;
                recibo=msgsnd(data.idBuzon,(struct msgbuf *)&mensajeReescrito, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), 0);
                if(recibo==-1){
                    perror("parking:msgSend");
                    exit(1);
                }
            }
            return 0;
        }
            
    }
}

int creaAlarm(){
    switch(fork()){
        case -1:
            perror("Parking:forkAlarm");
            exit(1);
        case 0:
        	data.parentPID=-1;
            alarm(30);
            pause();
            return 0;
    }
}

int creaChofers(){
    int i, recibo;
    struct PARKING_mensajeBiblioteca mensajeReescritoRec, mensajeOrden;
    data.pidChofers=malloc(data.numChoferes*sizeof(int));
    if(data.pidChofers==NULL){
    	perror("parking:malloc");
    	exit(1);
    }
    for(i=0; i<data.numChoferes;i++){
        switch(data.pidChofers[i]=fork()){
            case -1:
                perror("parking:fork");
                exit(1);
            case 0:
            data.parentPID=-1;
                while(1){
                    
                    if(data.prioridadAparcar==1 || data.prioridadDesaparcar==1){
                    	recibo=msgrcv(data.idBuzon,&mensajeReescritoRec, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), -2, 0);
						if(recibo==-1){
                        	perror("parking:reciboBuzon");
                        	exit(1);
						}
                    }else{
                    	recibo=msgrcv(data.idBuzon,&mensajeReescritoRec, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), MENSAJETIPO, 0);
						if(recibo==-1){
                        	perror("parking:reciboBuzon");
                        	exit(1);
						}
                    }
                    
                    if(mensajeReescritoRec.subtipo==PARKING_MSGSUB_APARCAR){
                        
                        char *dataMem;
                        //waitSem(FLAGSEMAFOROMEMORIA);
                        dataMem = shmat(data.idMemoria, (void *)0, 0);
                        if (dataMem == (char *)(-1)) {
                            perror("shmat");
                            exit(1);
                        }
                        
                        
                        if(PARKING_getNUmero(mensajeReescritoRec.hCoche)>1){
                        	switch(PARKING_getAlgoritmo(mensajeReescritoRec.hCoche)){
                        		case 0:
                        			recibo=msgrcv(data.idBuzon, &mensajeOrden, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long),PARKING_getNUmero( mensajeReescritoRec.hCoche)+PRIMERAJUSTEMENSAJES,0);
                        			if(recibo==-1){
                        				perror("parking:msgrcv");
                        				exit(1);
                        			}
                        			break;
                        		case 1:
                        			recibo=msgrcv(data.idBuzon, &mensajeOrden, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long),PARKING_getNUmero( mensajeReescritoRec.hCoche)+SIGUIENTEAJUSTEMENSAJES,0);
                        			if(recibo==-1){
                        				perror("parking:msgrcv");
                        				exit(1);
                        			}
                        			break;
                        		case 2:
                        			recibo=msgrcv(data.idBuzon, &mensajeOrden, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long),PARKING_getNUmero( mensajeReescritoRec.hCoche)+MEJORAJUSTEMENSAJES,0);
                        			if(recibo==-1){
                        				perror("parking:msgrcv");
                        				exit(1);
                        			}
                        			break;
                        		case 3:
                        			recibo=msgrcv(data.idBuzon, &mensajeOrden, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long),PARKING_getNUmero( mensajeReescritoRec.hCoche)+PEORAJUSTEMENSAJES,0);
                        			if(recibo==-1){
                        				perror("parking:msgrcv");
                        				exit(1);
                        			}
                        			break;
                        	}

                        }
                        
                        
                        PARKING_aparcar(mensajeReescritoRec.hCoche,NULL,(TIPO_FUNCION_APARCAR_COMMIT) aparcarCommit,
                                    (TIPO_FUNCION_PERMISO_AVANCE) permisoAvanzar,
                                    (TIPO_FUNCION_PERMISO_AVANCE_COMMIT) permisoAvanzarCommit);
                       
                                
                    }else{
                        PARKING_desaparcar(mensajeReescritoRec.hCoche,NULL,(TIPO_FUNCION_PERMISO_AVANCE) permisoAvanzar,
                                        (TIPO_FUNCION_PERMISO_AVANCE_COMMIT) permisoAvanzarCommit);
                        
                    }
                                //}//Llave while
                } //Llave switch
                    
                
        }
    }
}

void eliminaIPCS(){
    int error;
    
    if(data.parentPID==getpid()){
        error=semctl(data.idSemaforo,0,IPC_RMID);
        if(error==-1){
            perror("parking:semctlDelete");
            exit(1);
        }
        error=msgctl(data.idBuzon,IPC_RMID, NULL);
        if(error==-1){
            perror("parking:msgctlDelete");
            exit(1);
        }
        error=shmctl(data.idMemoria, IPC_RMID, NULL);
        if(error==-1){
            perror("parking:shmctlDelete");
            exit(1);
        }
    }
    
}

void creaIPCS(){
    int error, i;
    union semun s_u;
    s_u.val=1;
    data.idSemaforo=semget(IPC_PRIVATE,PARKING_getNSemAforos()+NUMSEMAFORO, IPC_CREAT | 0600);
    if(data.idSemaforo==-1){
    	perror("parking:semget");
    	exit(1);
    }
    
    data.idBuzon=msgget(IPC_PRIVATE,IPC_CREAT | 0600);
    if(data.idBuzon==-1){
    	perror("parking:msgget");
    	exit(1);
    }
    
    data.idMemoria=shmget(IPC_PRIVATE, PARKING_getTamaNoMemoriaCompartida()+(MEMSIZE*sizeof(char)), IPC_CREAT | 0600);
    if(data.idMemoria==-1){
    	perror("parking:shmget");
    	exit(1);
    }
    
    error=semctl(data.idSemaforo,PARKING_getNSemAforos(), SETVAL, s_u); //Inicializamos el semaforo para leer matriz.
    if(error==-1){
    	perror("parking:semctl");
    	exit(1);
    }

    if(error==-1){
        perror("parking:semctl");
        exit(1);
    } //SEGMENTATION FAULT!!
    for(i=POSSEMAFOROCARRETERAMEJOR; i<POSSEMAFOROCARRETERASIGUIENTE+ACERA; i++){
        error=semctl(data.idSemaforo, i, SETVAL,s_u);
        if(error==-1){
        	perror("parking:semctl");
        	exit(1);
        }
    }
            
    


}

void compruebaArg(int argc, char *argv[], int *debug){

    switch(argc){
        case 3:
            if(atoi(argv[1])<0){ //<0
            fprintf(stderr, "Error: Velocidad invalida. Esta debe ser mayor o igual que 0.\n");
            exit(1);
            }
            if(atoi(argv[2])<=0){ //<=0
                fprintf(stderr, "Error: Numero de choferes invalidos. Debe ser mayor que 0.\n");
                exit(1);
            }
            *debug=0;
            break;
        case 4:
            if(atoi(argv[1])<0){ //<0
            fprintf(stderr, "Error: Velocidad invalida. Esta debe ser mayor o igual que 0.\n");
            exit(1);
            }
            if(atoi(argv[2])<=0){ //<=0
                fprintf(stderr, "Error: Numero de choferes invalidos. Debe ser mayor que 0.\n");
                exit(1);
            }
            if((strcmp(argv[3],"PD") && strcmp(argv[3],"PA")) && strcmp(argv[3],"D")){ //!='D'
                fprintf(stderr, "Error: El tercer parametro debe de ser [PX] o [D]\nX puede ser 'D' (Prioridad para desaparcar) o 'A' (Prioridad para aparcar))\n");
                exit(1);
            }
            if(!strcmp(argv[3],"PD"))
            	data.prioridadDesaparcar=1;
            else if(!strcmp(argv[3],"PA"))
            	data.prioridadAparcar=1;

            if(!strcmp(argv[3],"D"))
                *debug=1;
            else
                *debug=0;
            break;
        case 5:
            if(atoi(argv[1])<0){ //<0
            fprintf(stderr, "Error: Velocidad invalida. Esta debe ser mayor o igual que 0.\n");
            exit(1);
            }
            if(atoi(argv[2])<=0){ //<=0
                fprintf(stderr, "Error: Numero de choferes invalidos. Debe ser mayor que 0.\n");
                exit(1);
            }
            if((strcmp(argv[3],"PD") || strcmp(argv[3],"PA"))){ //!='D'
                fprintf(stderr, "Error: El tercer parametro debe de ser [PX] o [D]\nX puede ser 'D' (Prioridad para desaparcar) o 'A' (Prioridad para aparcar))\n");
                exit(1);
            }
            if(strcmp(argv[4],"D")){
                fprintf(stderr, "Error: Argumentos invalidos.\nEl modo de uso es el siguiente: parking [velocidad][numChoferes][PX][D]\nX puede ser 'D' (Prioridad para desaparcar) o 'A' (Prioridad para aparcar))\n");
                exit(1);
            }
            *debug=1;
            if(!strcmp(argv[3],"PD"))
            	data.prioridadDesaparcar=1;
            else
            	data.prioridadAparcar=1;
            break;
        default:
            fprintf(stderr, "Error: Argumentos invalidos.\nEl modo de uso es el siguiente: parking [velocidad][numChoferes][PX][D]\nX puede ser 'D' (Prioridad para desaparcar) o 'A' (Prioridad para aparcar))\n");
            exit(1);
    }

}

void waitSem(int nSemaforo){
	int error;
    struct sembuf sops[1];
    sops[0].sem_num=nSemaforo;
    sops[0].sem_op=-1;
    sops[0].sem_flg=0;
    error=semop(data.idSemaforo, sops,1);
    if(error==-1){
    	perror("parking:semopWait");
    	exit(1);
    }
}

void signalSem(int nSemaforo){
	int error;
    struct sembuf sops[1];
    sops[0].sem_num=nSemaforo;
    sops[0].sem_op=1;
    sops[0].sem_flg=0;
    error=semop(data.idSemaforo, sops,1);
    if(error==-1){
    	perror("parking:semopSignal");
    	exit(1);
    }
}

void creaManejadoras(){
    int valorSigaction;
    //Manejadora SIGINT
    struct sigaction sigactionSIGINT;
    sigactionSIGINT.sa_handler=sigintHandler;
    sigactionSIGINT.sa_flags=0;
    valorSigaction=sigaction(SIGINT, &sigactionSIGINT, NULL);
    if(valorSigaction==-1){
        perror("parking:Sigaction_Sigint");
        exit(1);
    }

    struct sigaction sigactionSIGALRM;
    sigactionSIGALRM.sa_handler=sigalrmHandler;
    sigactionSIGALRM.sa_flags=0;
    valorSigaction=sigaction(SIGALRM, &sigactionSIGALRM, NULL);
    if(valorSigaction==-1){
        perror("parking:Sigaction_Sigalrm");
        exit(1);
    }


}


void sigintHandler(int sig){
    int i;
    PARKING_fin(0); 
    if(data.parentPID==getpid()){
        for(i=0; i<data.numChoferes; i++){
            kill(data.pidChofers[i],SIGINT );
        }
        kill(data.repartidorPID,SIGINT);
    }
    eliminaIPCS();
    clear();
    exit(1);
}

void sigalrmHandler(int sig){
    int i;
    kill(getppid(),SIGINT);
    exit(1);  
}



