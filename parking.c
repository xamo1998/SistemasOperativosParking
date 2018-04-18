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
#define MENSAJETIPO 66666
#define TIPOORDEN 99999
#define NUMSEMAFORO 1 + (ACERA*4)
#define PROFUNDIDAD 3
#define NUMDATOS 5
#define NUMFLAGS 1
#define MEMSIZE (PROFUNDIDAD*ACERA*4)+(NUMFLAGS)
#define ORDERCAR PARKING_getTamaNoMemoriaCompartida()+(ACERA*3)+(ACERA*3)+(ACERA*3)+1

#define HUECOMEJORPOS PARKING_getTamaNoMemoriaCompartida()
#define HUECOPRIMERPOS PARKING_getTamaNoMemoriaCompartida()+(ACERA*3)
#define HUECOPEORPOS PARKING_getTamaNoMemoriaCompartida()+(ACERA*3)+(ACERA*3)
#define HUECOSIGUIENTEPOS PARKING_getTamaNoMemoriaCompartida()+(ACERA*3)+(ACERA*3)+(ACERA*3)

#define CARRETERAMEJORPOS HUECOMEJORPOS+ACERA
#define CARRETERAPRIMERPOS HUECOPRIMERPOS+ACERA
#define CARRETERAPEORPOS HUECOPEORPOS+ACERA
#define CARRETERASIGUIENTEPOS HUECOSIGUIENTEPOS+ACERA

#define LINEAAPARCARMEJORPOS CARRETERAMEJORPOS+ACERA
#define LINEAAPARCARPRIMEROPOS CARRETERAPRIMERPOS+ACERA
#define LINEAAPARCARPEORPOS CARRETERAPEORPOS+ACERA
#define LINEAAPARCARSIGUIENTEPOS CARRETERASIGUIENTEPOS+ACERA

#define FLAGSEMAFOROMEMORIA PARKING_getNSemAforos()

#define POSSEMAFOROCARRETERAMEJOR FLAGSEMAFOROMEMORIA
#define POSSEMAFOROCARRETERAPRIMER POSSEMAFOROCARRETERAMEJOR + ACERA
#define POSSEMAFOROCARRETERAPEOR POSSEMAFOROCARRETERAPRIMER + ACERA
#define POSSEMAFOROCARRETERASIGUIENTE POSSEMAFOROCARRETERAPEOR + ACERA
//Variable globales
typedef struct Data{
	int  idSemaforo, idBuzon, idMemoria, *pidChofers, ipcsRemoved, numChoferes, parentPID;
	int huecoMejor[PROFUNDIDAD][ACERA], huecoPeor[PROFUNDIDAD][ACERA], huecoSiguiente[PROFUNDIDAD][ACERA], huecoPrimer[PROFUNDIDAD][ACERA];
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
void ChoferListener();
void creaIPCS();
int creaChofers();
int creaRepartidor();
void eliminaIPCS(); //Borrar
void imprimeMatriz(int algoritmo);
void waitSem(int nSemaforo);
void signalSem(int nSemaforo);
void inicializaMemoria();



int main(int argc, char *argv[]){
	int debug=0;
	data.parentPID=getpid();
	data.numChoferes=atoi(argv[2]);
	TIPO_FUNCION_LLEGADA f_llegadas[]={(TIPO_FUNCION_LLEGADA)primerAjuste,
									   (TIPO_FUNCION_LLEGADA)siguienteAjuste,
									   (TIPO_FUNCION_LLEGADA)mejorAjuste,
									   (TIPO_FUNCION_LLEGADA)peorAjuste};
	compruebaArg(argc, argv, &debug);
	
	creaManejadoras();
	creaIPCS();
	inicializaMemoria();
	PARKING_inicio(atoi(argv[1]), f_llegadas, data.idSemaforo, data.idBuzon, data.idMemoria, debug);
	creaAlarm();
	creaRepartidor();
	creaChofers();
	PARKING_simulaciOn();
	pause();
	//eliminaIPCS();
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

    for(i=PARKING_getTamaNoMemoriaCompartida();i<LINEAAPARCARSIGUIENTEPOS;i++)
    	dataMem[i]=0;

    if (shmdt(dataMem) == -1) {
    	perror("Parking:shmdt");
    }

}

TIPO_FUNCION_LLEGADA mejorAjuste(HCoche hc){
	return (TIPO_FUNCION_LLEGADA)-2;
}




TIPO_FUNCION_LLEGADA peorAjuste(HCoche hc){
	 return (TIPO_FUNCION_LLEGADA) -2;
    }






TIPO_FUNCION_LLEGADA siguienteAjuste(HCoche hc){
	return (TIPO_FUNCION_LLEGADA)-2;
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
	//waitSem(FLAGSEMAFOROMEMORIA);
	mensaje.tipo=hc+102;
	mensaje.subtipo=hc+1;
	mensaje.hCoche=hc;
	recibo=msgsnd(data.idBuzon,(struct msgbuf *)&mensaje, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), 0);
				if(recibo==-1)
					perror("parking:msgSend");
    //signalSem(FLAGSEMAFOROMEMORIA);
}

TIPO_FUNCION_PERMISO_AVANCE permisoAvanzar(HCoche hc){
	int i;
	
	//fprintf(stderr, "VENGO DE: {%d,%d} Y VOY A: {%d,%d} \n",PARKING_getX(hc),PARKING_getY(hc),PARKING_getX2(hc),PARKING_getY2(hc) );
	//sleep(1);
	//Accedemos a la memoria...
	char *dataMem;
	dataMem = shmat(data.idMemoria, (void *)0, 0);
    if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }	
    switch(PARKING_getAlgoritmo(hc)){
    	case 0: //Primer Ajuste...
		    //Realizamos la consulta...
    		
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
		    //}//Cuando salimos... Vemos opciones:
		  	if (shmdt(dataMem) == -1) {
		    	perror("Parking:shmdt");
		    }
		    return (TIPO_FUNCION_PERMISO_AVANCE)1; //Puede moverse
    		break; //End of primer ajuste.
    }
}

TIPO_FUNCION_PERMISO_AVANCE_COMMIT permisoAvanzarCommit(HCoche hc){
	int i;
	char *dataMem;
	//waitSem(FLAGSEMAFOROMEMORIA);
	dataMem = shmat(data.idMemoria, (void *)0, 0);
	if (dataMem == (char *)(-1)) {
	    perror("shmat");
	    exit(1);
	}
	switch(PARKING_getAlgoritmo(hc)){
		case 0:

			if(PARKING_getX(hc)==PARKING_getX2(hc)){//Ha sido un movimiento vertical.
				updateCarreteraVertical(PARKING_getAlgoritmo(hc), PARKING_getX(hc),PARKING_getX2(hc), PARKING_getY(hc),PARKING_getY2(hc), PARKING_getLongitud(hc));
			}else{
				updateCarreteraHorizontal(PARKING_getAlgoritmo(hc), PARKING_getX2(hc), PARKING_getLongitud(hc));
			}


				if(PARKING_getY2(hc)==1 && PARKING_getY(hc)==2){
					if(dataMem[HUECOPRIMERPOS+PARKING_getX(hc)])
					updateAcera(PARKING_getAlgoritmo(hc), PARKING_getX(hc), PARKING_getLongitud(hc));
			}
			

			    //Cerramos la memoria...
			    if (shmdt(dataMem) == -1) {
			    	perror("Parking:shmdt");
			    }
				//signalSem(FLAGSEMAFOROMEMORIA);
				
			break;
	}

}
void imprimeMatriz(int algoritmo){
	int i, j;
	char *dataMem;
	//waitSem(FLAGSEMAFOROMEMORIA);
	dataMem = shmat(data.idMemoria, (void *)0, 0);
    if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
	switch(algoritmo){
		case 0:
			/*fprintf(stderr, "\n\n\n\n\n\n\n\n\n\n\n\nLINEA:       ");
			for(i=0;i<ACERA;i++){
				fprintf(stderr, "%d",dataMem[LINEAAPARCARPRIMEROPOS+i]);
			}*/
			//sleep(1);
			fprintf(stderr, "\n\n\n\n\n\n\n\n\n\n\n\n\nCARRETERA:   ");
			for(i=0;i<ACERA;i++){
				fprintf(stderr, "%d",semctl(data.idSemaforo,POSSEMAFOROCARRETERAPRIMER+i,GETVAL));
			}
			//sleep(1);*/
			if (shmdt(dataMem) == -1) {
        		perror("Parking:shmdt");
    		}
    		//signalSem(FLAGSEMAFOROMEMORIA);
    		break;
	}
}
void updateCarreteraVertical(int algoritmo, int posActual, int posPrevia, int posVerticalActual,int posVerticalPrevia, int longitud){
	int i;
	char *dataMem;
	//waitSem(FLAGSEMAFOROMEMORIA);
	dataMem = shmat(data.idMemoria, (void *)0, 0);
	if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
	switch(algoritmo){
		case 0:
			for(i=posActual; i< (posActual+longitud); i++){
				//fprintf(stderr, "TENGO QQUE ACTUALIZAR DE %d a %d \n",posInicial, posInicial+longitud );
				//sleep(2);
				switch(posVerticalPrevia){
					case 2: //Vengo de la carretera...
						dataMem[CARRETERAPRIMERPOS+i]=0;
						dataMem[LINEAAPARCARPRIMEROPOS+i]=1;
						for(i=posActual; i<posActual+longitud; i++)
							signalSem(POSSEMAFOROCARRETERAPRIMER+i);
						break;
					case 1:
						if(posVerticalActual==0){//Si estoy en la carretera
							dataMem[CARRETERAPRIMERPOS+i]=1;
							dataMem[LINEAAPARCARPRIMEROPOS+i]=0;
						}else{ //Me voy a aparcar, luego pongo a 0 la linea
						dataMem[LINEAAPARCARPRIMEROPOS+i]=0;	
						}
						break;
					case 0:
						dataMem[LINEAAPARCARPRIMEROPOS+i]=1;
						break;

				}
			}
			if (shmdt(dataMem) == -1) {
        		perror("Parking:shmdt");
    		}
    		break;
	}
}


void updateCarreteraHorizontal(int algoritmo, int posInicial, int longitud){
	int i;
	char *dataMem;
	waitSem(FLAGSEMAFOROMEMORIA);
	dataMem = shmat(data.idMemoria, (void *)0, 0);
    if (dataMem == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }
	switch(algoritmo){
		case 0:
			if(posInicial<0){ //Estamos fuera de la pantalla	
				dataMem[CARRETERAPRIMERPOS+posInicial+longitud-1]=0;
				signalSem(POSSEMAFOROCARRETERAPRIMER+posInicial+longitud-1);//Ponemos libre la primera posicion de la carretera.
			}else if(posInicial==0){
				dataMem[CARRETERAPRIMERPOS+longitud-1]=0;
				signalSem(POSSEMAFOROCARRETERAPRIMER+longitud-1);
			}else{
				for(i=posInicial-1; i< (posInicial+longitud-1); i++){
					//fprintf(stderr, "TENGO QQUE ACTUALIZAR DE %d a %d \n",posInicial, posInicial+longitud );
					//sleep(2);
					if(i<ACERA){
						dataMem[CARRETERAPRIMERPOS+i]=1;
						
					}

					
				}
				//signalSem(POSSEMAFOROCARRETERAPRIMER+posInicial+longitud-1);
				if(posInicial+longitud-1<ACERA){
					dataMem[CARRETERAPRIMERPOS+posInicial+longitud-1]=0;
					signalSem(POSSEMAFOROCARRETERAPRIMER+posInicial+longitud-1);
				}
	    	}
	    	
	    	if (shmdt(dataMem) == -1) {
	        		perror("Parking:shmdt");
	    		}
	    	signalSem(FLAGSEMAFOROMEMORIA);
    		break;
	}
}
void updateAcera(int algoritmo, int posInicial, int longitud){
	int i;
	struct sembuf sops[1];
	char *dataMem;
	//waitSem(FLAGSEMAFOROMEMORIA);
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
    		//signalSem(FLAGSEMAFOROMEMORIA);
    		break;
		
		
	}
}


int creaRepartidor(){
	int recibo;
	struct PARKING_mensajeBiblioteca mensaje, mensajeReescrito;
	switch(fork()){
		case -1:
			perror("Parking:forkRepartidor");
			exit(1);
		case 0:
			while(1){
				recibo=msgrcv(data.idBuzon,&mensaje, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), PARKING_MSG, 0);
				if(recibo==-1)
					perror("parking:reciboBuzon");

				//Reescribir mensaje.
				mensajeReescrito.tipo=MENSAJETIPO;
				mensajeReescrito.subtipo=mensaje.subtipo;
				mensajeReescrito.hCoche=mensaje.hCoche;
				recibo=msgsnd(data.idBuzon,(struct msgbuf *)&mensajeReescrito, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), 0);
				if(recibo==-1)
					perror("parking:msgSend");
			}
			return 0;
	}
}

int creaAlarm(){
	switch(fork()){
		case -1:
			perror("Parking:forkAlarm");
			exit(1);
		case 0:
			alarm(200);
			pause();
			return 0;
	}
}

int creaChofers(){
	int i, recibo;
	struct PARKING_mensajeBiblioteca mensajeReescritoRec, mensajeOrden;
	data.pidChofers=malloc(data.numChoferes*sizeof(int));
	for(i=0; i<data.numChoferes;i++){
		switch(data.pidChofers[i]=fork()){
			case -1:
				perror("parking:fork");
				break;
			case 0:
				data.parentPID=-1;
				while(1){
					recibo=msgrcv(data.idBuzon,&mensajeReescritoRec, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long), MENSAJETIPO, 0);
					if(recibo==-1)
						perror("parking:reciboBuzon");
					//pon_error("He recibido un mensaje modificado\n");
					//printf("TIPO: %ld\n SUBTIPO: %ld\n HCoche: %i\n",mensaje.tipo, mensaje.subtipo, mensaje.hCoche);
					if(mensajeReescritoRec.subtipo==PARKING_MSGSUB_APARCAR){
						int datos[NUMDATOS];
						datos[0]=mensajeReescritoRec.hCoche;
						datos[1]=1; //Aparcar
						datos[2]=PARKING_getLongitud(mensajeReescritoRec.hCoche);
						//Confirmar el COMMIT

						if(mensajeReescritoRec.hCoche>0)
					    recibo=msgrcv(data.idBuzon, &mensajeOrden, sizeof(struct PARKING_mensajeBiblioteca)-sizeof(long),mensajeReescritoRec.hCoche+101,0);
					    	
    					PARKING_aparcar(mensajeReescritoRec.hCoche,datos,(TIPO_FUNCION_APARCAR_COMMIT) aparcarCommit,
                                    (TIPO_FUNCION_PERMISO_AVANCE) permisoAvanzar,
                                    (TIPO_FUNCION_PERMISO_AVANCE_COMMIT) permisoAvanzarCommit);
					   
								
					}else{
						int datos[NUMDATOS];
						datos[0]=mensajeReescritoRec.hCoche;
						datos[1]=0; //Aparcar
						datos[2]=PARKING_getLongitud(mensajeReescritoRec.hCoche);
						//Confirmar el COMMIT
    					PARKING_desaparcar(mensajeReescritoRec.hCoche,datos,(TIPO_FUNCION_PERMISO_AVANCE) permisoAvanzar,
                                        (TIPO_FUNCION_PERMISO_AVANCE_COMMIT) permisoAvanzarCommit);
						
					}
								//}//Llave while
				} //Llave switch
					
				
		}
	}
}

void eliminaIPCS(){
	int error;
	if(!data.ipcsRemoved){
		error=semctl(data.idSemaforo,0,IPC_RMID);
		if(error==-1){
			perror("parking:semctl");
			exit(1);
		}
		error=msgctl(data.idBuzon,IPC_RMID, NULL);
		if(error==-1){
			perror("parking:msgctl");
			exit(1);
		}
		error=shmctl(data.idMemoria, IPC_RMID, NULL);
		if(error==-1){
			perror("parking:shmctl");
			exit(1);
		}
		data.ipcsRemoved=1;
		printf("He eliminado todos los ipcs\n");
	}
	
}

void creaIPCS(){
	int error, i;
	union semun s_u;
	s_u.val=1;
	data.idSemaforo=semget(IPC_PRIVATE,PARKING_getNSemAforos()+NUMSEMAFORO, IPC_CREAT | 0600);
	
	data.idBuzon=msgget(IPC_PRIVATE,IPC_CREAT | 0600);
	
	data.idMemoria=shmget(IPC_PRIVATE, PARKING_getTamaNoMemoriaCompartida()+(MEMSIZE*sizeof(char)), IPC_CREAT | 0600);
	
	semctl(data.idSemaforo,PARKING_getNSemAforos(), SETVAL, s_u); //Inicializamos el semaforo para leer matriz.
	if(error==-1){
		perror("parking:semctl");
		exit(1);
	} //SEGMENTATION FAULT!!
	for(i=POSSEMAFOROCARRETERAMEJOR; i<POSSEMAFOROCARRETERASIGUIENTE; i++)
		semctl(data.idSemaforo, i, SETVAL, s_u);

	char *dataMem;
	dataMem = shmat(data.idMemoria, (void *)0, 0);
	if (dataMem == (char *)(-1)) {
	    perror("shmat");
	    exit(1);
	}
	dataMem[ORDERCAR]=-1;
	if (shmdt(dataMem) == -1) {
        perror("Parking:shmdt");
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
			break;
		default:
			fprintf(stderr, "Error: Argumentos invalidos.\nEl modo de uso es el siguiente: parking [velocidad][numChoferes][PX][D]\nX puede ser 'D' (Prioridad para desaparcar) o 'A' (Prioridad para aparcar))\n");
			exit(1);
	}

}

void waitSem(int nSemaforo){
	struct sembuf sops[1];
	sops[0].sem_num=nSemaforo;
	sops[0].sem_op=-1;
	sops[0].sem_flg=0;
	semop(data.idSemaforo, sops,1);
}

void signalSem(int nSemaforo){
	struct sembuf sops[1];
	sops[0].sem_num=nSemaforo;
	sops[0].sem_op=1;
	sops[0].sem_flg=0;
	semop(data.idSemaforo, sops,1);
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
	PARKING_fin(1);
	eliminaIPCS();
	if(data.parentPID==getpid()){
		for(i=0; i<data.numChoferes; i++){
			kill(SIGTERM, data.pidChofers[i]);
		}
	}
	exit(1);
}

void sigalrmHandler(int sig){
	int i;
	printf("Estoy en la manejadora de SIGALARM y han pasado 20 seg\n");
	PARKING_fin(1); //SI PONGO SEGMENTATION FAULT
	eliminaIPCS();
	for(i=0; i<data.numChoferes; i++){
		kill(SIGTERM, data.pidChofers[i]);
	}
}


