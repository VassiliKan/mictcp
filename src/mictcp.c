#include <mictcp.h>
#include <api/mictcp_core.h>

mic_tcp_sock my_socket;
mic_tcp_sock_addr sock_addr;
int nombre_sockets=0; //pour generer un identifiant unique propre au socket
int num_seq = 0;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(0);

    if (result==-1){
        printf("Erreur dans l'initialisation des components\n");
    } else{
        my_socket.fd=nombre_sockets; //identifiant du socket, doit etre unique
        nombre_sockets++;
        my_socket.state = IDLE;
        result = my_socket.fd;
    }
   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    if (my_socket.fd==mic_sock){
        mic_tcp_pdu pdu;

        pdu.header.source_port=my_socket.addr.port; /* numéro de port source */
        pdu.header.dest_port=my_socket.addr_dist.port; /* numéro de port de destination */
       
        
        pdu.header.seq_num = num_seq; /* numéro de séquence */
        //pdu.header.ack_num; /* numéro d'acquittement */
        //pdu.header.syn; /* flag SYN (valeur 1 si activé et 0 si non) */
        //pdu.header.ack; /* flag ACK (valeur 1 si activé et 0 si non) */
        //pdu.header.fin; /* flag FIN (valeur 1 si activé et 0 si non) */

        pdu.payload.size = mesg_size;
        pdu.payload.data = malloc (sizeof(char)*mesg_size);
        memcpy(pdu.payload.data, mesg, mesg_size);
        
        mic_tcp_pdu pdu_ack;
        IP_send(pdu, my_socket.addr_dist); 
        int result = IP_recv(&pdu_ack, NULL, 1000);
        while (result == -1 || (pdu_ack.header.ack=1 && pdu_ack.header.ack_num!=pdu.header.seq_num)); //expiration timer
        IP_send(pdu, my_socket.addr_dist); 
        result = IP_recv(&pdu_ack, NULL, 1000);
        
        return pdu.payload.size;
    }else{
        return -1;
    }
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{   
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu pdu;
    pdu.payload.data = mesg;
    pdu.payload.size = max_mesg_size;
    int mesg_size = app_buffer_get(pdu.payload);
    process_received_PDU(pdu,socket);
    return mesg_size;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    if (pdu.header.seq_num == num_seq){ //bien recu
        app_buffer_put(pdu.payload);

        mic_tcp_pdu pdu_ack;
        pdu_ack.header.source_port=my_socket.addr.port; /* numéro de port source */
        pdu_ack.header.dest_port=my_socket.addr_dist.port; /* numéro de port de destination */
        pdu_ack.header.ack_num = pdu.header.seq_num; /* numéro d'acquittement */
        pdu.header.ack=1; /* flag ACK (valeur 1 si activé et 0 si non) */
        IP_send(pdu_ack, my_socket.addr_dist); 
        printf("bien recu paquet %d\n", pdu_ack.header.ack_num);
    } else{ //numero d'acquittement mauvais
        mic_tcp_pdu pdu_ack;
        pdu_ack.header.source_port=my_socket.addr.port; /* numéro de port source */
        pdu_ack.header.dest_port=my_socket.addr_dist.port; /* numéro de port de destination */
        pdu_ack.header.ack_num = pdu.header.seq_num-1; /* numéro d'acquittement */
        pdu.header.ack=1; /* flag ACK (valeur 1 si activé et 0 si non) */
        IP_send(pdu_ack, my_socket.addr_dist); 
        printf("pas recu le bon paquet, on redemande paquet %d\n", pdu.header.seq_num);
    }
    
}
