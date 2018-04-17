#####Trabajo para sistemas operativos II#####

En esta pr�ctica vamos a simular, mediante procesos de UNIX, la gesti�n de memoria de un sistema operativo que use particiones de tama�o din�mico. Para ello, se establece el s�mil con el problema de elegir el mejor aparcamiento para un coche de todos los posibles en una acera. Seg�n se va ejecutando el programa, se ha de ver una imagen parecida a la siguiente: 

Captura de pantalla


El programa constar� de un �nico fichero fuente, parking.c, cuya adecuada compilaci�n producir� el ejecutable parking. Respetad las may�sculas/min�sculas de los nombres. 

Para simplificar la realizaci�n de la pr�ctica, se os proporciona una biblioteca est�tica de funciones (libparking.a) que deb�is enlazar con vuestro m�dulo objeto para generar el ejecutable. Gracias a ella, algunas de las funciones necesarias para realizar la pr�ctica no las tendr�is que programar sino que bastar� nada m�s con incluir la biblioteca cuando compil�is el programa. La l�nea de compilaci�n del programa podr�a ser:
gcc parking.c libparking.a -o parking -lm
Dispon�is, adem�s, de un fichero de cabeceras, parking.h, donde se encuentran definidas, entre otras cosas, las macros que usa la biblioteca. 

En la imagen puede observarse de color marr�n lo que simulan ser cuatro aceras. El algoritmo que se usa para aparcar los coches en cada una corresponde, de arriba a abajo, al primer ajuste, siguiente ajuste, mejor ajuste y peor ajuste, tal y como se vio anteriormente en la teor�a de Sistemas Operativos. Cada acera consta de ochenta caracteres (x=0 a x=79), numerados de izquierda a derecha. La coordenada y puede tomar valores entre 0 y 2, siendo 0 la calzada justo al lado de la acera, 1, la posici�n sobre las rayas de aparcamiento y 2, el carril de circulaci�n. Los coches tienen longitud entera, medida en caracteres. 

Quiz� de los cuatro algoritmos el que mayor dificultad presente sea el del siguiente ajuste. Cuando un coche llega a aparcar, de todos los huecos en los que cabe el coche, se elige el primero que aparece a partir de la posici�n en la que aparc� el �ltimo coche. Si se llega al final de la acera, se vuelve a empezar por el principio. El algoritmo no presenta mayor problema salvo cuando el �ltimo coche que aparc� ya se ha ido y tambi�n est� libre la posici�n actual. En ese caso, se empieza a buscar por el comienzo del hueco donde se halla la posici�n actual. 

En el caso de los algoritmos del mejor ajuste y del peor ajuste, si hay empate entre dos o m�s huecos, se elegir� entre ellos siguiendo el criterio del primer ajuste. 

Una vez elegido un hueco con espacio suficiente para aparcar el coche, se aparcar� ajustando el coche a la izquierda del hueco en cualquiera de los algoritmos. 

Si cuando llega un coche, no existe un hueco de tama�o suficiente para que aparque, el coche se pondr� en una cola a la espera de que otro coche salga del aparcamiento. Esta cola seguir� una disciplina FIFO. 

Los coches tienen un n�mero que los identifica. Los coches deben aparcar en orden consecutivo. Vuestro programa debe garantizar que el orden se respeta, pues de no hacer nada, algunos coches podr�an adelantarse debido al reparto de CPU. Tambi�n tienen los coches un color y una velocidad propias. 

Para que en la forma de desarrollarse de cada algoritmo no influya la suerte, en cada ejecuci�n concreta llegar�n los mismos coches, con id�ntico tiempo de servicio e intervalo de llegada entre ellos a cada una de las cuatro aceras. Para el tiempo de servicio y el intervalo de llegada entre coches, se usar� una distribuci�n exponencial. Para la longitud de los coches, la distribuci�n ser� uniforme, con una longitud comprendida 7 y 17 caracteres. 

Para facilitar la tarea, ten�is a vuestra disposici�n una biblioteca de funciones de enlazado est�tico libparking.a y un fichero de cabeceras, parking.h. Gracias a la biblioteca, muchas de las funciones no las tendr�is que programar sino que invocarlas a la biblioteca. Tambi�n estar� a cargo de la biblioteca muchos de los detalles de la pr�ctica: llegadas y salidas de coches, gesti�n de la cola de espera, etc. Una descripci�n detallada de las funciones de la biblioteca aparece m�s abajo en esta misma p�gina. 

Adem�s del proceso padre, necesitar�is crear algunos procesos hijos m�s. En concreto, existir�n unos procesos ch�feres que se dedicar�n a aparcar y desaparcar los coches. Estos procesos siguen una estrategia denominada cuadrilla de trabajadores (pool of workers). Consiste en que se quedan en espera a que les llegue una petici�n de aparcar o desaparcar un coche. Una vez realizada la tarea, vuelven a quedarse a la espera de recibir otra solicitud. Tambi�n existe un hijo adicional del proceso padre que se encargar� de contar el tiempo que dura la simulaci�n e invocar a la funci�n PARKING_fin. Es posible que necesit�is, adicionalmente, un proceso gestor para tramitar los mensajes recibidos de la biblioteca a trav�s del buz�n. 

El programa, desde vuestro punto de vista, se simplifica bastante. El proceso padre ha de:
comprobar los argumentos pasados por la l�nea de �rdenes
registrar las manejadoras
crear los recursos IPCs necesarios: array de sem�foros, buz�n y zona de memoria compartida
llamar a la funci�n PARKING_inicio
crear los procesos ch�feres y el proceso que avisa
invocar la funci�n PARKING_simulaciOn


Para que el programa funcione correctamente, es necesario crear un array de sem�foros, un buz�n de paso de mensajes y una zona de memoria compartida privados. Parte de estos recursos los necesita la biblioteca y el resto que reserv�is los pod�is usar para sincronizar y comunicar los procesos entre s�. En concreto, hay dos funciones auxiliares que os indican cu�ntos sem�foros necesita la biblioteca (PARKING_getNSemAforos) y cu�ntos bytes de memoria compartida requiere la biblioteca (PARKING_getTamaNoMemoriaCompartida). Tanto los sem�foros como la zona de memoria usados por la biblioteca estar�n situados al inicio del recurso reservado. 

Por su parte, la vida de los procesos ch�feres transcurre pl�cida y tranquila, como ya se ha explicado. Duermen hasta recibir una petici�n de aparcar o desaparcar e invocan las funciones PARKING_aparcar y PARKING_desaparcar, respectivamente. Solamente deben tener cuidado de no llamar a la funci�n de aparcamiento hasta que los coches con un n�mero inferior al suyo ya la hayan invocado y recibido confirmaci�n de la biblioteca. 

Cuando hay que aparcar o desaparcar un coche, la biblioteca manda un mensaje al buz�n de tipo PARKING_MSG y subtipos PARKING_MSGSUB_APARCAR y PARKING_MSGSUB_DESAPARCAR, respectivamente. El formato de estos mensajes se puede deducir de la definici�n de struct PARKING_mensajeBiblioteca que se encuentra en el fichero parking.h. 

De haber varias peticiones sin atender, dependiendo de c�mo se haya invocado la pr�ctica, se debe seguir una pol�tica:
FIFO (no se especifica nada en la l�nea de �rdenes)
de prioridad para los aparcamientos (especificamos PA)
de prioridad para desaparcar (se especifica PD)
Para lograrlo, se puede usar un proceso gestor que tramite los mensajes, los reescriba, renumere y reenv�e o, cualquier otro m�todo que logre los resultados que se pretenden. 

Cada coche que participa en la simulaci�n tiene un manejador opaco de tipo HCoche para poder trabajar con �l. Existen funciones que permiten conocer las caracter�sticas de los coches mediante su manejador. Un listado exhaustivo se ofrece m�s abajo. 

Se deben programar cuatro funciones, una por algoritmo, que ser�n llamadas por la biblioteca cada vez que un coche llegue. Dichas funciones de rellamada son registradas en la funci�n PARKING_inicio de la biblioteca. 

El prototipo de las funciones de rellamada se describe aqu�:
int funciOn_llegada(HCoche hc); 
La biblioteca llamar� a una de estas funciones cuando llegue un nuevo coche. La respuesta que debe devolver vuestro c�digo indicar� qu� se debe hacer con ese coche. -1 indica que no hay sitio para �l y debe encolarse. Un valor entre 0 y 79 significa la posici�n a partir de la cual se debe aparcar el coche. Adem�s, si devolvemos -2, indicaremos a la biblioteca que no queremos procesar m�s coches de ese algoritmo, lo que puede ser �til para anular uno o varios algoritmos para depurar los dem�s.


La funciones PARKING_aparcar y PARKING_desaparcar son especiales puesto que gestionan el movimiento de los coches autom�ticamente mediante funciones de rellamada a vuestro c�digo. Estos son sus prototipos con la correspondiente explicaci�n:
int PARKING_aparcar(HCoche,void *datos, TIPO_FUNCION_APARCAR_COMMIT, TIPO_FUNCION_PERMISO_AVANCE, TIPO_FUNCION_PERMISO_AVANCE_COMMIT; 
La biblioteca puede llamar a la funci�n de llegada en cualquier orden y es responsabilidad del programador que los coches se aparquen en orden num�rico. Para ello, se debe invocar la funci�n PARKING_aparcar del coche que toque y no invocar a la funci�n del coche siguiente hasta haber recibido la rellamada de aparcar commit. Esta rellamada indica que la biblioteca ha tomado nota de la anterior llamada y ya se puede invocar al siguiente. 

El avance del coche tambi�n se regula mediante dos funciones de rellamada que se especifican en esta funci�n. Cada vez que un coche quiere avanzar, la biblioteca invoca a nuestra funci�n de permiso avance. Debemos bloquearnos, sin consumir CPU, hasta que el avance sea seguro. En esos momentos, la biblioteca produce el avance en el coche y nos avisa a trav�s de la funci�n permiso avance commit.
int PARKING_desaparcar(HCoche,void *datos, TIPO_FUNCION_PERMISO_AVANCE, TIPO_FUNCION_PERMISO_AVANCE_COMMIT); 
La descripci�n de esta funci�n es equivalente a la de la funci�n anterior, con la salvedad de que, al no importar el orden en que se desaparca, no es necesaria una funci�n de confirmaci�n (commit) en este caso.


Caracter�sticas adicionales que programar
El programa no debe consumir CPU apreciablemente en los modos de retardo mayor o igual que 1. para comprobar el consumo de CPU, pod�is usar la orden top.
IMPORTANTE: Aunque no se indique expl�citamente en el enunciado, parece obvio que se necesitar�n objetos de sincronizaci�n en diferentes partes del programa.
Se permite hacer semiespera ocupada cuando un ch�fer espera a que le toque a su coche aparcar, pero de hacerse as�, la nota puede verse rebajada.
El tiempo de ejecuci�n de la pr�ctica ser� de 30 segundos. El proceso encargado de llevar la cuenta no puede usar la llamada sleep, sino hacerlo mediante la se�al SIGALRM.


Parking acepta un m�nimo de dos y un m�ximo de cuatro argumentos por la l�nea de �rdenes. Si no se introducen argumentos, se imprimir� un mensaje con la forma de uso del programa por el canal de error est�ndar. En el caso de teclear dos argumentos, el primer argumento ser� un n�mero entero mayor o igual que cero relacionado con la rapidez con que se producen los acontecimientos en el programa. Cero indica la m�xima rapidez y n�meros mayores suponen una rapidez progresivamente menor. El segundo argumento es el n�mero de ch�feres que se usar�n en esta ejecuci�n. Finalmente, si son m�s de dos los argumentos introducidos, los dos primeros son id�nticos al caso anterior y el resto podr� ser una combinaci�n cualquiera de:
D, indicando que se desea que el programa produzca informaci�n de depuraci�n por el canal de errores est�ndar
PA, para que se d� prioridad a los que aparcan
PD, para que se d� prioridad a los que desaparcan
La �nica combinaci�n que no se permitir� ser� PA y PD a la vez, por contradictoria. Por ejemplo, para una velocidad 10, cinco ch�feres, con informaci�n de depuraci�n y prioridad para los que desaparcan, se podr�a invocar as�: parking 10 5 PD D 

El programa debe estar preparado para que, si el usuario pulsa las teclas CTRL-C desde el terminal, la ejecuci�n del programa termine en ese momento y adecuadamente. Ni en una terminaci�n como esta, ni en una normal, deben quedar procesos en ejecuci�n ni mecanismos IPC sin haber sido borrados del sistema. Este es un aspecto muy importante y se penalizar� bastante si la pr�ctica no lo cumple. 

Es probable que necesit�is sem�foros para sincronizar adecuadamente la pr�ctica. Se declarar� una array de sem�foros de tama�o adecuado a vuestros requerimientos, los primeros de los cuales se reservar�n para el funcionamiento interno de la biblioteca, como se indic� m�s arriba. El resto, pod�is usarlos libremente. Del mismo modo, la memoria compartida reservada se divide en dos partes, la primera de las cuales es para uso interno de la biblioteca. 

Las funciones proporcionadas por la biblioteca libparking.a son las que a continuaci�n aparecen. De no indicarse nada, las funciones devuelven -1 en caso de error:
int PARKING_inicio(int ret, TIPO_FUNCION_LLEGADA f_llegadas[], int semAforos, int buzOn, int zona, int debug) 
El proceso padre debe llamar a esta funci�n cuando desee que la simulaci�n comience. La funci�n devuelve -1 si se ha producido un error. Los argumentos son:
ret: lentitud de ejecuci�n. Es el valor que se ha pasado como primer argumento al programa.
f_llegadas: es un array de cuatro elementos. Contiene los punteros a las funciones de vuestro programa que quer�is que la biblioteca invoque cada vez que llega un coche nuevo para cada algoritmo. El prototipo de las funciones se ha especificado m�s arriba.
semAforos: identificador del array de sem�foros devuelto por semget.
buzOn: idem para el buz�n de paso de mensajes.
zona: idem para la zona de memoria compartida (no es el puntero, sino lo que devuelve shmget).
debug: flag que indica si se desea que la biblioteca produzca informaci�n de depuraci�n por el canal de errores est�ndar.
void PARKING_aparcar(HCoche hc,void *datos, void (*fCommit)(HCoche), void (*fPermisoAvance)(HCoche), void (*fPermisoAvanceCommit)(HCoche)) 
Se invoca esta funci�n cuando queremos que aparezca por la pantalla la animaci�n de un coche aparcando y siempre respetando el orden num�rico de los coches:
hc: manejador del coche que queremos aparcar.
datos: puntero de datos que podemos asociar al coche para recuperarlos en cualquier otro momento.
fCommit: manejadora que ser� invocada cuando la biblioteca haya anotado la salida del coche.
fPermisoAvance: manejadora que es llamada por la biblioteca antes de efectuar un movimiento del coche para aparcar. A trav�s del manejador del coche que se nos pasa, podremos obtener su informaci�n. En particular, su posici�n (funciones PARING_getX() y PARKING_getY()), la posici�n a la que desea ir (funciones PARING_getX2() y PARKING_getY2()), el puntero a los datos que hemos asociado al coche (funci�n PARKING_getDatos()), etc. La funci�n se debe bloquear sin consumo de CPU hasta que sea seguro para el coche el avance.
fPermisoAvanceCommit: esta manejadora ser� invocada por la biblioteca cuando ha hecho efectivo el avance del coche. En este caso, las coordenadas (x,y) se corresponden con la posici�n ya avanzada del coche y (x2,y2) son las coordenadas de la posici�n antes de avanzar.
void PARKING_desaparcar(HCoche hc,void *datos, void (*fPermisoAvance)(HCoche), void (*fPermisoAvanceCommit)(HCoche)) 
Id�ntica a la funci�n anterior, s�lo que para que se muestre la salida de un coche. En este caso no aparece la funci�n commit pues se hace innecesaria, al no ser un requisito que desaparquen en un determinado orden.
int PARKING_getNUmero(HCoche hc)
int PARKING_getLongitud(HCoche hc)
int PARKING_getPosiciOnEnAcera(HCoche hc)
int PARKING_getColor(HCoche hc)
int PARKING_getX(HCoche hc)
int PARKING_getY(HCoche hc)
int PARKING_getX2(HCoche hc)
int PARKING_getY2(HCoche hc)
int PARKING_getAlgoritmo(HCoche hc)
unsigned long PARKING_getTServ(HCoche hc)
void *PARKING_getDatos(HCoche hc)

Todas estas funciones dan informaci�n relativa al coche cuyo manejador se pasa como par�metro. Dicha informaci�n es, respectivamente:
n�mero del coche
longitud del coche
posici�n x en la acera donde ese coche va a aparcar o -1 si todav�a no est� decidido
color del coche
coordenada x de la posici�n del coche donde se encuentra
lo mismo para la coordenada y
coordenada x de la posici�n a la que quiere avanzar el coche (funci�n de permiso de avance) o de la que viene (funci�n de confirmaci�n de permiso de avance)
idem coordenada y
algoritmo al que pertenece el coche (PRIMER_AJUSTE, SIGUIENTE_AJUSTE, MEJOR_AJUSTE y PEOR_AJUSTE, seg�n vienen definidas en el fichero de cabeceras)
tiempo de servicio en unidades de tiempo de la simulaci�n
puntero de datos que se le ha asignado al coche en la funci�n de aparcar
int PARKING_isAceraOcupada(int algoritmo,int pos) 
Devuelve verdadero si la posici�n (0-79) de la acera (0-3) indicadas en sus argumentos est� ocupada por un coche, desde el punto de vista de la biblioteca. Solamente debe usarse como depuraci�n. El estado de las aceras en cada momento lo debe gestionar vuestra aplicaci�n con sus propias variables.
int PARKING_getTamaNoMemoriaCompartida()
int PARKING_getNSemAforos()
Estas funciones devuelven los requerimientos de la biblioteca respecto a la memoria compartida (bytes) o n�mero de sem�foros
int PARKING_fin(int tipo) 
Se debe llamar a esta funci�n cuando se desee terminar la simulaci�n. Si se trata de una terminaci�n normal, se le pasar� un 1. Si es por un error, se le pasar� un cero.
void pon_error(char *mensaje) 
Pone un mensaje de error en la parte inferior de la pantalla y espera a que el usuario pulse "Intro". La pod�is usar para depurar.


Estad atentos pues pueden ir saliendo versiones nuevas de la biblioteca para corregir errores o dotarla de nuevas funciones. 

Respecto a la sincronizaci�n interna de la biblioteca, se usa un sem�foro reservado para conseguir atomicidad en la actualizaci�n de la pantalla (semPantalla, iniciado a 1), otro sem�foro para evitar consumo de CPU innecesario en la simulaci�n (semSim, iniciado a 0) y cuatro sem�foros adicionales para secciones cr�ticas para cada uno de los cuatro algoritmos (semAlgoritmo[], iniciados a 1). Para que las sincronizaciones que de seguro deber�is hacer en vuestro c�digo est�n en sinton�a con las de la biblioteca, os ofrecemos ahora un seudoc�digo de algunas de las funciones que realiza la biblioteca y est�n reguladas por los sem�foros.
    * PARKING_inicio:
         - da valores iniciales a los recursos IPC de la biblioteca
         - dar valor inicial a los par�metros de la simulaci�n
         - iniciar el generador de n�meros aleatorios
         - limpiar la pantalla, poner mensaje inicial y dibujar

    * PARKING_fin:
         - si es normal, marcar acabando
         - si es por error, hacer que el cursor sea visible

    * PARKING_simulaciOn:
         - comienzo del bucle de simulaci�n
         -   c�lculo del pr�ximo suceso (llegada o salida)
         -   dormir hasta que llegue el suceso o se avise mediante semSim
         -   avanzar el reloj de la simulaci�n
         -   para todos los algoritmos a
         -     W(semAlgoritmo[a])
         -       pasar los reci�n aparcados a la lista de aparcados
         -       destruir los coches reci�n desaparcados
         -     S(semAlgoritmo[a])
         -   si toca acabar, salir del bucle
         -   para todos los algoritmos a
         -     mandar mensaje por cada coche cuyo tiempo de espera ha expirado (desaparcar)
         -   si estamos acabando, continuar el bucle
         -   mientras el tiempo de llegada del pr�ximo coche sea menor que
               el reloj actual, crear un nuevo coche y encolarlo en la lista de espera
         -   para todos los coches que est�n en la lista de espera
         -     W(semAlgoritmo[a])
         -       llamar a la funci�n de llegada del algoritmo para ese coche
         -       si la posici�n devuelta es -2
                   poner t_siguiente llegada muy alto; S(semAlgoritmo[a]); break
         -       si la posici�n calculada es incorrecta
                   poner el error; S(semAlgoritmo[a]); break
         -       si la posici�n calculada es -1
                   S(semAlgoritmo[a]); break
         -       dibujar la reserva
         -       mandar mensaje de aparcar
         -     S(semAlgoritmo[a])
         - fin del bucle de la simulaci�n

    * PARKING_aparcar:
         - si no le toca a este coche, poner el error
         - W(semAlgoritmo[a]); incrementar el n�mero que toca; S(semAlgoritmo[a]);
         - almacenar el puntero de los datos de usuario pasado en el coche
         - desde la pos=79 hasta la posici�n en que el coche tiene que aparcarse
         -   pausa de avance
         -   llamar a permiso de avance
         -   dibujar el coche
         -   llamar a permiso de avance commit
         -   si pos=79, llamar a aparcar commit
         - hacer lo mismo para los dos avances verticales para acabar de aparcar
         - W(semAlgoritmo[a])
         -   encolar el coche en los reci�n aparcados
         -   avisar con semSim
         - S(semAlgoritmo[a])

    * PARKING_desaparcar:
         - permiso avance, dibujo y llamar a avance commit para el primer avance vertical
         - permiso avance para el segundo avance vertical
         - W(semAlgoritmo[a])
         -   dibujar el coche
         -   liberar la reserva de la acera
         -   llamar a permiso avance commit
         - S(semAlgoritmo[a])
         - hasta que el coche desaparece: pausa, permiso avance, dibujo, llamar a avance commit
         - W(semAlgoritmo[a])
         -   meter el coche en la lista de reci�n desaparcados
         -   avisar con semSim
         - S(semAlgoritmo[a])
    
En esta pr�ctica no se podr�n usar ficheros para nada, salvo que se indique expresamente. Las comunicaciones de PIDs o similares entre procesos, si hicieran falta, se har�n mediante mecanismos IPC. 

Siempre que en el enunciado o LPEs se diga que se puede usar sleep(), se refiere a la llamada al sistema, no a la orden de la l�nea de �rdenes. 

Los mecanismos IPC (sem�foros, memoria compartida y paso de mensajes) son recursos muy limitados. Es por ello, que vuestra pr�ctica s�lo podr� usar un conjunto de sem�foros, un buz�n de paso de mensajes y una zona de memoria compartida como m�ximo. Adem�s, si se produce cualquier error o se finaliza normalmente, los recursos creados han de ser eliminados. Una manera f�cil de lograrlo es registrar la se�al SIGINT para que lo haga y mand�rsela uno mismo si se produce un error. 

Biblioteca de funciones libparking.a
Con esta pr�ctica se trata de que aprend�is a sincronizar y comunicar procesos en UNIX entre s�. Su objetivo no es la programaci�n, aunque es inevitable que teng�is que programar. Es por ello que se os suministra una biblioteca est�tica de funciones ya programadas para tratar de que no deb�is preocuparos por la presentaci�n por pantalla, la gesti�n de estructuras de datos (colas, pilas, ...) , etc. Tambi�n servir� para que se detecten de un modo autom�tico errores que se produzcan en vuestro c�digo. Para que vuestro programa funcione, necesit�is la propia biblioteca libparking.a y el fichero de cabecera parking.h. La biblioteca funciona con los c�digos de VT100/xterm, por lo que deb�is adecuar vuestros simuladores a este terminal.
Ficheros necesarios:
libparking.a: para Solaris (ver 2.2), para el LINUX de clase (ver 2.2),
parking.h: Para todos (ver 2.0).

Pasos recomendados para la realizaci�n de la pr�ctica
Aunque ya deber�ais ser capaces de abordar la pr�ctica sin ayuda, aqu� van unas gu�as generales:
Hacer el c�digo necesario para gestionar los argumentos que se le pasan al programa
Crear los sem�foros, la memoria comparida y el buz�n, y comprobad que se crean bien, con ipcs. Es preferible, para que no haya interferencias, que los defin�is privados.
Registrar SIGINT para que cuando se pulse ^C se eliminen los recursos IPC. Lograr que si el programa acaba normalmente o se produce cualquier error, tambi�n se eliminen los recursos (mandad una se�al SIGINT en esos casos al proceso padre).
Llamar a la funci�n PARKING_inicio en main. Debe aparecer la pantalla de bienvenida y, pasados dos segundos, dibujarse la pantalla. Para el array de funciones de llegada, usad funciones que nada m�s tengan un pause() dentro, de momento. Todos los procesos que se creen a continuaci�n en sucesivos puntos deben crearse despu�s de haber llamado a PARKING_inicio.
Dejad a padre en pause() y cread el hijo que se encarga de contar el tiempo y avisa al padre llamando a PARKING_fin(). Recordad que ten�is que usar SIGALRM para eso y no sleep
A continuaci�n, haced que el padre llame a PARKING_simulaciOn()
Los siguientes pasos conllevan resolver la pr�ctica solo para un algoritmo y con un ch�fer, para ir, poco a poco, aumentando la dificultad. Para ello, solo programad la funci�n de llegada del primer algoritmo. El resto de funciones de llegada, que devuelvan -2 para indicar a la biblioteca que no quer�is, de momento, tratar con ellas. Y el ch�fer, que solo lea el mensaje y lo imprima en la pantalla.
Que el ch�fer llame ahora a PARKING_aparcar. Pod�is hacer que las funciones de commits y permisos, de momento, solo pongan un mensaje de depuraci�n.
Que el ch�fer distinga entre los dos tipos de mensajes y llame a PARKING_aparcar o PARKING_desaparcar, seg�n corresponda. Ya deber�ais ser capaces de ver a los coches aparcar y salir, cuando toque.
Las cosas se van a complicar ahora, cuando a�ad�is varios ch�feres: puede haber errores de orden de aparcamiento y de choques. Deb�is introducir mecanismos de sincronizaci�n para que todo funcione.
Acabad la pr�ctica y probadla a velocidad normal y a velocidad cero.
Meted la opci�n de diferentes pol�ticas de gesti�n de la cola de tareas de los ch�feres: FIFO, prioridad de aparcamientos y prioridad de desaparcamientos.
A�adid el resto de algoritmos
Dise�ad la forma de acabar sin problemas y llamad a la funci�n PARKING_fin().
Pulid los �ltimos detalles.


Plazo de presentaci�n.
Consultad la p�gina de entrada de la asignatura. 

Normas de presentaci�n.
Ac� est�n. Adem�s de estas normas, en esta pr�ctica se debe entregar un esquema donde aparezcan los sem�foros usados, sus valores iniciales, los buzones, y mesajes pasados y un seudoc�digo sencillo para cada proceso con las operaciones wait y signal, send y receive realizadas sobre ellos. Por ejemplo, si se tratara de sincronizar dos procesos C y V para que produjeran alternativamente consonantes y vocales, comenzando por una consonante, deber�ais entregar algo parecido a esto:
     SEM�FOROS Y VALOR INICIAL: SC=1, SV=0.

     SEUDOC�DIGO:

             C                                V
            ===                              ===
       Por_siempre_jam�s               Por _siempre_jam�s
          {                               {
           W(SC)                           W(SV)
           escribir_consonante             escribir_vocal
           S(SV)                           S(SC)
           }                               }
Daos cuenta de que lo que importa en el pseudoc�digo es la sincronizaci�n. El resto puede ir muy esquem�tico. Un buen esquema os facilitar� much�simo la defensa. 

Evaluaci�n de la pr�ctica.
Dada la dificultad para la correcci�n de programaci�n en paralelo, el criterio que se seguir� para la evaluaci�n de la pr�ctica ser�: si
la pr�ctica cumple las especificaciones de este enunciado y,
la pr�ctica no falla en ninguna de las ejecuciones a las que se somete y,
no se descubre en la pr�ctica ning�n fallo de construcci�n que pudiera hacerla fallar, por muy remota que sea esa posibilidad...
se aplicar� el principio de "presunci�n de inocencia" y la pr�ctica estar� aprobada. La nota, a partir de ah�, depender� de la simplicidad de las t�cnicas de sincronizaci�n usadas, la correcci�n en el tratamiento de errores, la cantidad y calidad del trabajo realizado, etc. 

LPEs.
�Se puede usar la biblioteca en un Linux de 64 bits? Aqu� se os indican las claves.
�Se puede proporcionar la biblioteca para el Sistema Operativo X, procesador Y? Por problemas de eficiencia en la gesti�n y mantenimiento del c�digo no se proporcionar� la biblioteca m�s que para Solaris-SPARC y Linux de 32 bits. A veces pod�is lograr encontrar una soluci�n mediante el uso de m�quinas virtuales.
�D�nde poner un sem�foro? Dondequiera que us�is la frase, "el proceso puede llegar a esperar hasta que..." es un buen candidato a que aparezca una operaci�n wait sobre un sem�foro. Ten�is que plantearos a continuaci�n qu� proceso har� signal sobre ese presunto sem�foro, d�nde lo har� y cu�l ser� su valor inicial.
Si ejecut�is la pr�ctica en segundo plano (con ampersand (&)) es normal que al pulsar CTRL+C el programa no reaccione. El terminal s�lo manda SIGINT a los procesos que est�n en primer plano. Para probarlo, mandad el proceso a primer plano con fg % y pulsad entonces CTRL+C.
Un "truco" para que sea menos penoso el tratamiento de errores consiste en dar valor inicial a los identificadores de los recursos IPC igual a -1. Por ejemplo, int semAforo=-1. En la manejadora de SIGINT, s�lo si semAforo vale distinto de -1, elimin�is el recurso con semctl. Esto es l�gico: si vale -1 es porque no se ha creado todav�a o porque al intentar crearlo la llamada al sistema devolvi� error. En ambos casos, no hay que eliminar el recurso.
Para evitar que todos los identificadores de recursos tengan que ser variables globales para que los vea la manejadora de SIGINT, pod�is declarar una estructura que los contenga a todos y as� s�lo gast�is un identificador del espacio de nombres globales.
A muchos os da el error "Interrupted System Call". Mirad la sesi�n dedicada a las se�ales, apartado quinto. All� se explica lo que pasa con wait. A vosotros os pasa con semop, pero es lo mismo. De las dos soluciones que propone el apartado, deb�is usar la segunda.
A muchos, la pr�ctica os funciona exasperantemente lenta en encina. Deb�is considerar que la m�quina cuando la prob�is est� cargada, por lo que debe ir m�s lento que en casa o en el linux de clase.
A aquellos que os d� "Bus error (Core dumped)" al dar valor inicial al sem�foro, considerad que hay que usar la versi�n de semctl de Solaris (con union semun), como se explica en la sesi�n de sem�foros.
Al acabar la pr�ctica, con CTRL+C, al ir a borrar los recursos IPC, puede ser que os ponga "Invalid argument", pero, sin embargo, se borren bien. La raz�n de esto es que hab�is registrado la manejadora de SIGINT para todos los procesos. Al pulsar CTRL+C, la se�al la reciben todos, el padre y los otros procesos. El primero que obtiene la CPU salta a su manejadora y borra los recursos. Cuando saltan los dem�s, intentan borrarlos, pero como ya est�n borrados, os da el error.
El compilador de encina tiene un bug. El error t�picamente os va a ocurrir cuando defin�is una variable entera en memoria compartida. Os va a dar Bus Error. Core dumped si no defin�s el puntero a esa variable apuntando a una direcci�n que sea m�ltiplo de cuatro. El puntero que os devuelve shmat, no obstante, siempre ser� una direcci�n m�ltiplo de cuatro, por lo que solo os ten�is que preocupar con que la direcci�n sea m�ltiplo de cuatro respecto al origen de la memoria compartida. La raz�n se escapa un poco al nivel de este curso y tiene que ver con el alineamiento de direcciones de memoria en las instrucciones de acceso de palabras en el procesador RISC de encina.
Todos vosotros, tarde o temprano, os encontr�is con un error que no tiene explicaci�n: un proceso que desaparece, un sem�foro que parece no funcionar, etc. La actitud en este caso no es tratar de justificar la imposibilidad del error. As� no lo encontrar�is. Ten�is que ser muy sistem�ticos. Hay un �rbol entero de posibilidades de error y no ten�is que descartar ninguna de antemano, sino ir podando ese �rbol. Ten�is que encontrar a los procesos responsables y tratar de localizar la l�nea donde se produce el error. Si el error es "Segmentation fault. Core dumped", la l�nea os la dar� si aplic�is lo que aparece en la secci�n Manejo del depurador. En cualquier otro caso, no os quedar� m�s remedio que depurar mediante �rdenes de impresi�n dentro del c�digo. 

Para ello, insertad l�neas del tipo:
                     fprintf(stderr,"%d:...",getpid(),...);
donde sospech�is que hay problemas. En esas l�neas identificad siempre al proceso que imprime el mensaje. Comprobad todas las hip�tesis, hasta las m�s evidentes. Cuando ejecut�is la pr�ctica, redirigid el canal de errores a un fichero con 2>salida. 

Si cada proceso pone un identificador de tipo "P1", "P2", etc. en sus mensajes, pod�is quedaros con las l�neas que contienen esos caracteres con:
                     grep "P1" salida > salida2
Si aparecen errores de acceso a memoria, especialmente a partir de la versi�n 2 de la biblioteca y en la fuci�n isValidHandle, puede ser porque creais los procesos auxiliares antes de llamar a PARKING_inicio. Todos los procesos se deben crear despu�s de llamar a PARKING_inicio.
