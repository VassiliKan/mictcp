#include <mictcp.h>
#include <api/mictcp_core.h>

#define LOSS_RATE 20
#define LOSS_WINDOW_SIZE 10
#define MAX_TRY_CONNECT 15
#define TIMER 15
#define NUM_SOCKET 10

protocol_state client_state;
protocol_state serveur_state;               

// socket
mic_tcp_sock my_socket;
mic_tcp_sock_addr sock_addr;

//numéros de séquence et d'acquittement
int seq_num = 0;
int ack_num = 0;

// variables relatives aux pertes
char *taux_pertes_negociations = "10";
int nb_send = 0;
int effective_loss_rate = 0;
int taux_pertes_admissibles = 0;

// Fenêtre glissante comptabilisant les pertes; 0 = OK, 1 = NOK
int loss_window[LOSS_WINDOW_SIZE]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int loss_window_index = 0;

// Mutex et condition pour la fonction accept()
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm) {
    
    int result;

    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf(" mode %d", sm); printf("\n");
   
    result = initialize_components(sm); /* Appel obligatoire */

    /*
    * A décommenter si l'on veut obtenir une fiabilité totale sur les trames d'acquittement, le taux de pertes effectif global est alors égal à LOSS_RATE
    *
    if(sm == CLIENT){
        set_loss_rate(LOSS_RATE);
    } else {
        set_loss_rate(0);
    }*/

    set_loss_rate(LOSS_RATE);

    if (result==-1){
        printf("Erreur dans l'initialisation des components\n");
        exit(1);
    } else {
        my_socket.fd = NUM_SOCKET; //identifiant du socket, doit etre unique
        my_socket.state = IDLE;
        result = my_socket.fd;
    }
    
    return result;
}


/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr) {
 
    int result = -1;

    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    if(my_socket.fd == socket){
        memcpy(&my_socket.addr, &addr, sizeof(mic_tcp_sock_addr));
        result = 0;
    }

    return result;
}


/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr) {
    
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    if(my_socket.fd != socket){
        return -1;
    }

    my_socket.state = WAIT_FOR_CONNECTION;

    if(pthread_mutex_lock(&mutex) < 0){
        printf("ERROR LOCK MUTEX ACCEPT\n");
    }

    while(my_socket.state != CONNECTED){
        pthread_cond_wait(&cond, &mutex);
    }

    if(pthread_mutex_unlock(&mutex) < 0){
        printf("ERROR UNLOCK MUTEX ACCEPT\n");
    }

    return 0;
}


/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) {

    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    mic_tcp_pdu pdu_send;
    mic_tcp_pdu pdu_recv;
    int res_recv;
    int nb_try = 0;
    int is_not_synack = 1;
    int msg_size = sizeof(char) * strlen(taux_pertes_negociations);       // fois 3 car taux est maximum composé de trois chiffres

    if (my_socket.fd != NUM_SOCKET){
        return -1;
    }

    pdu_send.header.source_port = my_socket.addr.port; 
    pdu_send.header.dest_port = my_socket.addr_dist.port; 
    pdu_send.header.syn = 1;
    pdu_send.payload.size = msg_size;
    pdu_send.payload.data = malloc (msg_size);
    memcpy(pdu_send.payload.data, taux_pertes_negociations, msg_size);
    
    do {
        if(IP_send(pdu_send, my_socket.addr_dist) < 0){
            printf("ERROR SEND SYN\n");
        } 
        res_recv = IP_recv(&pdu_recv, NULL, TIMER);

        my_socket.state = WAIT_FOR_SYNACK;

        if(pdu_recv.header.syn == 1 && pdu_recv.header.ack == 1){
            is_not_synack = 0;
        }

        nb_try++;

    } while (is_not_synack && nb_try < MAX_TRY_CONNECT);

    pdu_send.header.syn = 0;
    pdu_send.header.ack = 1;

    if(IP_send(pdu_send, my_socket.addr_dist) < 0){
        printf("ERROR SEND ACK\n");
    } 

    my_socket.state = CONNECTED;

    return 0;
}


/*
 * Calcule le taux de pertes sur la fenêtre glissante 
 *  Renvoie un entier correspondant à ce taux (en %)
 */
int calcul_loss_rate(){
    int loss = 0;

    for (int i = 0; i < LOSS_WINDOW_SIZE; i++){             // boucle sur le tableau des derniers envois et calcul du loss rate
        loss += loss_window[i];
        //printf("%d", loss_window[i]);
    }

    return loss * 100 / LOSS_WINDOW_SIZE;
}


/*
 * Appelle fonction de calcul du taux de pertes sur la fenêtre glissante
 * Décide de renvoyer ou non le paquet en comparant le taux de pertes effectif à celui admissible
 */
int decideRetry(){
    int retry;
    loss_window[loss_window_index] = 1;

    effective_loss_rate = calcul_loss_rate();

    // Cas trop de pertes, on renvoie
    if(effective_loss_rate > taux_pertes_admissibles){ 
        printf("RETRY\n");                
        retry = 1;
    } else {
        printf("ABANDON\n");
        retry = 0;
    }

    return retry;
}


/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size) {
    
    mic_tcp_pdu pdu_send;
    mic_tcp_pdu pdu_recv = {0};
    int res_send;
    int res_recv;
    int retry = 0;

    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    if (my_socket.fd != mic_sock){
        return -1;
    }

    pdu_send.header.source_port = my_socket.addr.port; 
    pdu_send.header.dest_port = my_socket.addr_dist.port; 
    pdu_send.header.seq_num = seq_num; 
    pdu_send.payload.size = mesg_size;
    pdu_send.payload.data = malloc (sizeof(char)*mesg_size);
    memcpy(pdu_send.payload.data, mesg, mesg_size);
   
    nb_send++;
    
    do {
        if (loss_window_index == LOSS_WINDOW_SIZE) { // remise a 0 de l'index pour eviter un debordement d'entier, alternative au modulo
            loss_window_index = 0;
        }

        res_send = IP_send(pdu_send, my_socket.addr_dist); 
        res_recv = IP_recv(&pdu_recv, NULL, TIMER);

        printf("ENVOI %d ", nb_send);
        if(res_recv == -1) {                                    // expiration timer
            printf("ECHEC TIMER : ");
            retry = decideRetry();
        } else if (pdu_recv.header.ack_num != seq_num){         // reception du mauvais numero d'acquittement 
            printf("ECHEC ACK != SEQ : ");
            retry = decideRetry();
            if (!retry) {
                seq_num = (seq_num + 1) % 2;
            }
        } else {                                                // succès envoi
            printf("SUCCES\n");
            loss_window[loss_window_index] = 0;
            seq_num = (seq_num + 1) % 2;
            retry = 0;
        }

        loss_window_index++;

    } while (retry);

    return res_send;
}


/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size) {   
     
    mic_tcp_pdu pdu;

    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    pdu.payload.data = mesg;
    pdu.payload.size = max_mesg_size;

    return app_buffer_get(pdu.payload);
}


/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr) {
 
    mic_tcp_pdu pdu_send;

    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");


    switch(my_socket.state){

        case WAIT_FOR_CONNECTION:
            if(pdu.header.syn == 1){ 
                pdu_send.header.syn = 1;
                pdu_send.header.ack = 1;

                // Fixe le taux de pertes admissibles, défini dans la varaible taux_pertes_negociations

                taux_pertes_admissibles = atoi(pdu.payload.data);

                my_socket.state = WAIT_FOR_ACK;
            }
            break;

        case WAIT_FOR_ACK:
            if(pdu.header.ack == 1){
                if(pthread_cond_broadcast(&cond) < 0){
                    printf("ERROR CONDITION BROADCAST\n");
                }
                my_socket.state = CONNECTED;
            }
            break;

        case CONNECTED:
            if(pdu.header.seq_num == ack_num) {
                app_buffer_put(pdu.payload);
            } 

            pdu_send.header.ack_num = ack_num;

            ack_num = (ack_num + 1 ) % 2;

            break;

        default:
            break;
    }

    pdu_send.header.source_port = my_socket.addr.port;
    pdu_send.header.dest_port = addr.port;

    IP_send(pdu_send, addr); // my_socket.addr_dist); // addr emetteur pas en dur

}   
        
  
/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket) {

    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    
    return 0;
}