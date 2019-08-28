#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void* service( void* aClientSocket )
{
    if( aClientSocket == nullptr )
        pthread_exit(0);

    printf( "%s \n", "Start service" );

    int client = *( ( int* )aClientSocket );

    char* buffer = new char[1024];
    while( true )
    {
        int numberOfAccepdetBytes = read( client, ( void* )&buffer, sizeof( buffer ) );
        printf( "Accepted bytes : %d \n", numberOfAccepdetBytes );

        if( buffer == nullptr )
            break;

        int numberOfSendetBytes = write( client, ( void* )&buffer, sizeof( buffer ) );
        printf( "Sended bytes : %d \n", numberOfSendetBytes );

        memset(&buffer, 0, sizeof( buffer ) );
    }

    printf( "%s \n", "Stop service" );

    delete buffer;
    close( client );
    pthread_exit( 0 );
}

void server( int aListenSocket, sockaddr_in aListenSocketAddr )
{
    printf( "%s \n", "Strart initialize server" );

    int         maxEvents = 10;
    epoll_event event;
    epoll_event events[maxEvents];

    int listenSocket  = aListenSocket;
    int connectSocket = 0;
    int nfds          = 0;
    int epollfd       = 0;

    pthread_t thread;

    // Create epoll fd
    epollfd = epoll_create1(0);

    if ( epollfd == -1 )
    {
        perror( "epoll_create1" );
        exit( EXIT_FAILURE );
    }

    event.events  = EPOLLIN;
    event.data.fd = listenSocket;

    // Add listen socket to queue
    if ( epoll_ctl( epollfd, EPOLL_CTL_ADD, listenSocket, &event ) == -1 )
    {
        perror( "epoll_ctl: listen_sock" );
        exit( EXIT_FAILURE );
    }

    printf( "%s \n", "The server was successfully initialized" );
    printf( "%s \n", "The server is running" );
    printf( "%s \n", "==============================================" );

    socklen_t length = sizeof( sockaddr_in );
    for ( ;; )
    {
        // Number of sockets ready to receive / transmit
        nfds = epoll_wait( epollfd, events, maxEvents, -1 );

        if ( nfds == -1 )
        {
            perror( "epoll_wait" );
            exit( EXIT_FAILURE );
        }

        for ( int n = 0; n < nfds; ++n )
        {
            if ( events[n].data.fd == listenSocket )
            {
                // Accept client to free socket from queue
                connectSocket = accept( listenSocket,
                                        ( struct sockaddr* )&aListenSocketAddr,
                                        &length
                                      );

                if ( connectSocket == -1 )
                {
                    perror( "accept" );
                    exit( EXIT_FAILURE );
                }

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = connectSocket;

                // Add new socket to epoll queue
                if ( epoll_ctl( epollfd, EPOLL_CTL_ADD, connectSocket, &event ) == -1 )
                {
                    perror( "epoll_ctl: conn_sock" );
                    exit( EXIT_FAILURE );
                }
            }
            else
            {
                pthread_create( &thread, 0, service, (void*)&connectSocket );
                pthread_detach( thread );
            }
        }
    }

    close( epollfd );
}

int main()
{
    int         listenSocket     = 0;
    int         listenSocketPort = 54000;
    int         maxConnections   = 5;
    sockaddr_in listenSockAddr;

    // Create listen socket
    listenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );    

    if( listenSocket <= 0 )
    {
        printf( "%s \n", "Error...: Can not create socket!" );
        return -1;
    }

    // Configure listen socket address
    listenSockAddr.sin_family      = AF_INET;
    listenSockAddr.sin_port        = htons( listenSocketPort );
    listenSockAddr.sin_addr.s_addr = INADDR_ANY;    

    // Bind listen socket to port
    if( bind( listenSocket, ( sockaddr* )&listenSockAddr, ( socklen_t )sizeof( sockaddr_in ) ) < 0 )
    {
        printf( "%s \n", "Error...: Can not bind socket to port" );
        return -2;
    }

    // Mark listen socket to listen
    if( listen( listenSocket, maxConnections ) < 0 )
    {
        printf( "%s \n", "Error...: Server can not listen" );
        return -3;
    }

    printf( "%s \n", "The listen socket was successfully initialized" );

    // Run main server part
    server( listenSocket, listenSockAddr );

    return 0;
}
