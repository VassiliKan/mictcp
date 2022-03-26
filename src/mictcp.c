#include <mictcp.h>
#include <string.h>
#include <api/mictcp_core.h>

mic_tcp_sock my_socket;
mic_tcp_sock_addr sock_addr;
int num_socket = 10; //pour generer un identifiant unique propre au socket

int seq_num = 0;
int ack_num = 0;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm) {
    
   int result;

   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(0);

    if (result==-1){
        printf("Erreur dans l'initialisation des components\n");
        exit(1);
    } else{
        my_socket.fd = num_socket; //identifiant du socket, doit etre unique
        my_socket.state = IDLE;
        result = my_socket.fd;
        num_socket++;
    }
   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr) {
 
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr) {
  
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) {
    
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size) {
    
    mic_tcp_pdu pdu_send;
    mic_tcp_pdu pdu_recv;
    int result;
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

    do {
        result = IP_send(pdu_send, my_socket.addr_dist); 
        //printf("ENVOIE SEQ NUMERO %d\n", seq_num);
        if(IP_recv(&pdu_recv, NULL, 100) == -1) {
            retry = 1;
        } else if (pdu_recv.header.ack_num != seq_num) {
            retry = 1;
        } else {
            retry = 0;
            seq_num = (seq_num + 1) % 2;
        }
    } while (retry);
    
    //seq_num = (seq_num + 1) % 2;
    
    return result;
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
    
    mic_tcp_pdu pdu_ack;

    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    if(pdu.header.seq_num == ack_num) {
        app_buffer_put(pdu.payload);
    } 

    pdu_ack.header.source_port = my_socket.addr.port;
    pdu_ack.header.dest_port = my_socket.addr_dist.port;
    pdu_ack.header.ack_num = ack_num;

    IP_send(pdu_ack, my_socket.addr_dist);
    ack_num = (ack_num + 1 ) % 2;

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
