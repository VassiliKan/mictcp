#include <mictcp.h>
#include <api/mictcp_core.h>

mic_tcp_sock my_socket;
mic_tcp_sock_addr sock_addr;
int nombre_sockets=0; //pour generer un identifiant unique propre au socket
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
    if (my_socket.fd==socket){
        my_socket.addr=addr;
        my_socket.addr.ip_addr = malloc(addr.ip_addr_size*sizeof(char));
        memcpy(my_socket.addr.ip_addr,addr.ip_addr, addr.ip_addr_size*sizeof(char));
        return 0;
    }else{
        return -1;
    }
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return -1;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return -1;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    if (my_socket.fd=mic_sock){
        mic_tcp_pdu pdu;

        pdu.header.source_port=my_socket.addr.port; /* numéro de port source */
        pdu.header.dest_port=my_socket.addr_dist.port; /* numéro de port de destination */
        pdu.header.seq_num=0; /* numéro de séquence */
        pdu.header.ack_num=0; /* numéro d'acquittement */
        pdu.header.syn=0; /* flag SYN (valeur 1 si activé et 0 si non) */
        pdu.header.ack=0; /* flag ACK (valeur 1 si activé et 0 si non) */
        pdu.header.fin=0;

        pdu.payload.size = mesg_size;
        pdu.payload.data = malloc (sizeof(char)*mesg_size);
        memcpy(pdu.payload.data, mesg, mesg_size);

        int result = IP_send(pdu, my_socket.addr_dist); 
        return pdu.payload.mesg_size;
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
    return -1;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return -1;
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
}
