//=============================================================================
// Copyright © 2014 NaturalPoint, Inc. All Rights Reserved.
// 
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall NaturalPoint, Inc. or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//=============================================================================

/*

PacketClient.cpp

Decodes NatNet packets directly.

Usage [optional]:

	PacketClient [ServerIP] [LocalIP]

	[ServerIP]			IP address of server ( defaults to local machine)
	[LocalIP]			IP address of client ( defaults to local machine)

*/

#include <stdio.h>

#define RETURNTYPE void*
#define SOCKET int
#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#define closesocket close
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define SOCKADDR const struct sockaddr
#define NAMEFLAG 0

#define MAX_NAMELENGTH              256

// NATNET message ids
#define NAT_PING                    0 
#define NAT_PINGRESPONSE            1
#define NAT_REQUEST                 2
#define NAT_RESPONSE                3
#define NAT_REQUEST_MODELDEF        4
#define NAT_MODELDEF                5
#define NAT_REQUEST_FRAMEOFDATA     6
#define NAT_FRAMEOFDATA             7
#define NAT_MESSAGESTRING           8
#define NAT_UNRECOGNIZED_REQUEST    100
#define UNDEFINED                   999999.9999

#define MAX_PACKETSIZE				100000	// max size of packet (actual packet size is dynamic)

// sender
typedef struct
{
    char szName[MAX_NAMELENGTH];            // sending app's name
    unsigned char Version[4];               // sending app's version [major.minor.build.revision]
    unsigned char NatNetVersion[4];         // sending app's NatNet version [major.minor.build.revision]

} sSender;

typedef struct
{
    unsigned short iMessage;                // message ID (e.g. NAT_FRAMEOFDATA)
    unsigned short nDataBytes;              // Num bytes in payload
    union
    {
        unsigned char  cData[MAX_PACKETSIZE];
        char           szData[MAX_PACKETSIZE];
        unsigned long  lData[MAX_PACKETSIZE/4];
        float          fData[MAX_PACKETSIZE/4];
        sSender        Sender;
    } Data;                                 // Payload

} sPacket;


bool IPAddress_StringToAddr(char *szNameOrAddress, struct in_addr *Address);
void Unpack(char* pData);
int GetLocalIPAddresses(unsigned long Addresses[], int nMax);

#define MULTICAST_ADDRESS		"239.255.42.99"     // IANA, local network
#define PORT_COMMAND            1510
#define PORT_DATA  			    1511                // Default multicast group

SOCKET CommandSocket;
SOCKET DataSocket;
in_addr ServerAddress;
sockaddr_in HostAddr;  

int NatNetVersion[4] = {0,0,0,0};
int ServerVersion[4] = {0,0,0,0};

// command response listener thread
RETURNTYPE CommandListenThread(void* dummy)
{
    socklen_t addr_len;
    socklen_t nDataBytesReceived;

    char str[256];
    sockaddr_in TheirAddress;
    sPacket PacketIn;
    addr_len = sizeof(struct sockaddr);

	printf("Command Thread started\n");

    while (1)
    {
        // blocking
        nDataBytesReceived = recvfrom( CommandSocket,(char *)&PacketIn, sizeof(sPacket),
            0, (struct sockaddr *)&TheirAddress, &addr_len);

        if((nDataBytesReceived == 0) || (nDataBytesReceived == SOCKET_ERROR) )
            continue;

        // debug - print message
        printf("[Client] Received command from %s: Command=%d, nDataBytes=%d",
		inet_ntoa( TheirAddress.sin_addr ), (int)PacketIn.iMessage, (int)PacketIn.nDataBytes);



        // handle command
        switch (PacketIn.iMessage)
        {
        case NAT_MODELDEF:
            Unpack((char*)&PacketIn);
            break;
        case NAT_FRAMEOFDATA:
            Unpack((char*)&PacketIn);
            break;
        case NAT_PINGRESPONSE:
            for(int i=0; i<4; i++)
            {
                NatNetVersion[i] = (int)PacketIn.Data.Sender.NatNetVersion[i];
                ServerVersion[i] = (int)PacketIn.Data.Sender.Version[i];
            }
            break;
        case NAT_RESPONSE:
            {
                char* szResponse = (char *)PacketIn.Data.cData;
                printf("Response : %s",szResponse);
                break;
            }
        case NAT_UNRECOGNIZED_REQUEST:
            printf("[Client] received 'unrecognized request'\n");
            break;
        case NAT_MESSAGESTRING:
            printf("[Client] Received message: %s\n", PacketIn.Data.szData);
            break;
        }
    }

    return 0;
}

// Data listener thread
RETURNTYPE DataListenThread(void* dummy)
{
	printf("Data listen thread started\n");
    char  szData[20000];
    sockaddr_in TheirAddress;
    socklen_t addr_len = sizeof(struct sockaddr);
    socklen_t nDataBytesRecieved;

    while (1)
    {
        // Block until we receive a datagram from the network (from anyone including ourselves)
        int nDataBytesReceived = recvfrom(DataSocket, szData, sizeof(szData), 0, (sockaddr *)&TheirAddress, &addr_len);
	printf("x");
        Unpack(szData);
    }

    return 0;
}

SOCKET CreateCommandSocket(unsigned long IP_Address, unsigned short uPort)
{
    struct sockaddr_in my_addr;     
    static unsigned long ivalue;
    static unsigned long bFlag;
    int nlengthofsztemp = 64;  
    SOCKET sockfd;

    // Create a blocking, datagram socket
    if ((sockfd=socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
	printf("Could not create a datagram socket\n");
        return -1;
    }

    // bind socket
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(uPort);
    //my_addr.sin_addr.s_addr = IP_Address;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
	printf("Unable to bind command socket\n");
        closesocket(sockfd);
        return -1;
    }

    // set to broadcast mode
    ivalue = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&ivalue, sizeof(ivalue)) == SOCKET_ERROR)
    {
	printf("Unable to set command socket options\n");
        closesocket(sockfd);
        return -1;
    }

    return sockfd;
}

int main(int argc, char* argv[])
{
    int retval;
    char szMyIPAddress[128] = "";
    char szServerIPAddress[128] = "";
    in_addr MyAddress, MultiCastAddress;
    int optval = 0x100000;

    socklen_t optval_size = 4;
    pthread_t thread;

	// server address
	if(argc>1)
	{
		strcpy(szServerIPAddress, argv[1]);	// specified on command line
	    retval = IPAddress_StringToAddr(szServerIPAddress, &ServerAddress);
	}
	else
	{
        GetLocalIPAddresses((unsigned long *)&ServerAddress, 1);
        sprintf(szServerIPAddress, "%s", inet_ntoa(ServerAddress));
	}

    // client address
	if(argc>2)
	{
		strcpy(szMyIPAddress, argv[2]);	// specified on command line
	    retval = IPAddress_StringToAddr(szMyIPAddress, &MyAddress);
	}
	else
	{
        GetLocalIPAddresses((unsigned long *)&MyAddress, 1);
	sprintf(szMyIPAddress, "%s", inet_ntoa(MyAddress));
    }
  	//MultiCastAddress.S_un.S_addr = inet_addr(MULTICAST_ADDRESS);   
  	MultiCastAddress.s_addr = inet_addr(MULTICAST_ADDRESS);   
    printf("Client: %s\n", szMyIPAddress);
    printf("Server: %s\n", szServerIPAddress);
    printf("Multicast Group: %s\n", MULTICAST_ADDRESS);

    // create "Command" socket
    int port = 0;
    CommandSocket = CreateCommandSocket(MyAddress.s_addr,port);
    printf("Command Socket: %x\n", CommandSocket);
    if(CommandSocket == -1)
    {
        // error
	printf("Error creating command socket\n");
    }
    else
    {
        // [optional] set to non-blocking
        //u_long iMode=1;
        //ioctlsocket(CommandSocket,FIONBIO,&iMode); 
        // set buffer
        setsockopt(CommandSocket, SOL_SOCKET, SO_RCVBUF, (char *)&optval, 4);
        getsockopt(CommandSocket, SOL_SOCKET, SO_RCVBUF, (char *)&optval, &optval_size);
        if (optval != 0x100000)
        {
            // err - actual size...
        }
	printf("Creating command thread...\n");
	pthread_create( &thread, 0, &CommandListenThread, 0 );
    }

    // create a "Data" socket
    DataSocket = socket(AF_INET, SOCK_DGRAM, 0);

    // allow multiple clients on same machine to use address/port
    int value = 1;
    retval = setsockopt(DataSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value));
    if (retval == SOCKET_ERROR)
    {
        closesocket(DataSocket);
        return -1;
    }

    struct sockaddr_in MySocketAddr;
    memset(&MySocketAddr, 0, sizeof(MySocketAddr));
    MySocketAddr.sin_family = AF_INET;
    MySocketAddr.sin_port = htons(PORT_DATA);
    MySocketAddr.sin_addr = MyAddress; 
    if (bind(DataSocket, (struct sockaddr *)&MySocketAddr, sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
	printf("[PacketClient] bind failed\n");
        return 0;
    }
    // join multicast group
    struct ip_mreq Mreq;
    Mreq.imr_multiaddr = MultiCastAddress;
    Mreq.imr_interface = MyAddress;
    retval = setsockopt(DataSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&Mreq, sizeof(Mreq));
    if (retval == SOCKET_ERROR)
    {
	printf("[PacketClient] join failed\n");
        return -1;
    }
	// create a 1MB buffer
    setsockopt(DataSocket, SOL_SOCKET, SO_RCVBUF, (char *)&optval, 4);
    getsockopt(DataSocket, SOL_SOCKET, SO_RCVBUF, (char *)&optval, &optval_size);
    if (optval != 0x100000)
    {
        printf("[PacketClient] ReceiveBuffer size = %d\n", optval);
    }

	pthread_t DataListenThread_Handle;
	printf("creating data listen thread...\n");
	pthread_create( &DataListenThread_Handle, NULL, &DataListenThread, NULL );
    
    // server address for commands
    memset(&HostAddr, 0, sizeof(HostAddr));
    HostAddr.sin_family = AF_INET;        
    HostAddr.sin_port = htons(PORT_COMMAND); 
    HostAddr.sin_addr = ServerAddress;

    // send initial ping command
    sPacket PacketOut;
    PacketOut.iMessage = NAT_PING;
    PacketOut.nDataBytes = 0;
    int nTries = 3;
    while (nTries--)
    {
        int iRet = sendto(CommandSocket, (char *)&PacketOut, 4 + PacketOut.nDataBytes, 0, (sockaddr *)&HostAddr, sizeof(HostAddr));
        if(iRet != SOCKET_ERROR)
            break;
    }


    printf("Packet Client started\n\n");
    printf("Commands:\ns\tsend data descriptions\nf\tsend frame of data\nt\tsend test request\nq\tquit\n\n");
    int c;
    char szRequest[512];
    bool bExit = false;
    nTries = 3;
    while (!bExit)
    {
		c = getchar();
        switch(c)
        {
        case 's':
            // send NAT_REQUEST_MODELDEF command to server (will respond on the "Command Listener" thread)
            PacketOut.iMessage = NAT_REQUEST_MODELDEF;
            PacketOut.nDataBytes = 0;
            nTries = 3;
            while (nTries--)
            {
                int iRet = sendto(CommandSocket, (char *)&PacketOut, 4 + PacketOut.nDataBytes, 0, (sockaddr *)&HostAddr, sizeof(HostAddr));
                if(iRet != SOCKET_ERROR)
                    break;
            }
            break;	
        case 'f':
            // send NAT_REQUEST_FRAMEOFDATA (will respond on the "Command Listener" thread)
            PacketOut.iMessage = NAT_REQUEST_FRAMEOFDATA;
            PacketOut.nDataBytes = 0;
            nTries = 3;
            while (nTries--)
            {
                int iRet = sendto(CommandSocket, (char *)&PacketOut, 4 + PacketOut.nDataBytes, 0, (sockaddr *)&HostAddr, sizeof(HostAddr));
                if(iRet != SOCKET_ERROR)
                    break;
            }
            break;	
        case 't':
            // send NAT_MESSAGESTRING (will respond on the "Command Listener" thread)
            strcpy(szRequest, "TestRequest");
            PacketOut.iMessage = NAT_REQUEST;
            PacketOut.nDataBytes = (int)strlen(szRequest) + 1;
            strcpy(PacketOut.Data.szData, szRequest);
            nTries = 3;
            while (nTries--)
            {
                int iRet = sendto(CommandSocket, (char *)&PacketOut, 4 + PacketOut.nDataBytes, 0, (sockaddr *)&HostAddr, sizeof(HostAddr));
                if(iRet != SOCKET_ERROR)
                    break;
            }
            break;	
        case 'p':
            // send NAT_MESSAGESTRING (will respond on the "Command Listener" thread)
            strcpy(szRequest, "Ping");
            PacketOut.iMessage = NAT_PING;
            PacketOut.nDataBytes = (int)strlen(szRequest) + 1;
            strcpy(PacketOut.Data.szData, szRequest);
            nTries = 3;
            while (nTries--)
            {
                int iRet = sendto(CommandSocket, (char *)&PacketOut, 4 + PacketOut.nDataBytes, 0, (sockaddr *)&HostAddr, sizeof(HostAddr));
                if(iRet != SOCKET_ERROR)
                    break;
            }
            break;	
        case 'q':
            bExit = true;		
            break;	
        default:
            break;
        }
    }

    return 0;
}


// convert ipp address string to addr
bool IPAddress_StringToAddr(char *szNameOrAddress, struct in_addr *Address)
{
	int retVal;
	struct sockaddr_in saGNI;
	char hostName[256];
	char servInfo[256];
	u_short port;
	port = 0;

	// Set up sockaddr_in structure which is passed to the getnameinfo function
	saGNI.sin_family = AF_INET;
	saGNI.sin_addr.s_addr = inet_addr(szNameOrAddress);
	saGNI.sin_port = htons(port);

	// getnameinfo
	if ((retVal = getnameinfo((SOCKADDR *)&saGNI, sizeof(sockaddr), hostName, 256, servInfo, 256, NAMEFLAG)) != 0)
	{
        printf("[PacketClient] GetHostByAddr failed.\n");
		return false;

	}

    Address->s_addr = saGNI.sin_addr.s_addr;

    return true;
}

// get ip addresses on local host
int GetLocalIPAddresses(unsigned long Addresses[], int nMax)
{
    char szMyName[1024];
    struct addrinfo aiHints;
	struct addrinfo *aiList = NULL;
    struct sockaddr_in addr;
    int retVal = 0;
    const char* port = "0";
    
    size_t  NameLength = 128;
	retVal = gethostname(szMyName, NameLength);
	if(retVal == -1)
	{
		printf("[PacketClient] get computer name  failed.");
		return 0;
	}

	memset(&aiHints, 0, sizeof(aiHints));
	aiHints.ai_family = AF_INET;
	aiHints.ai_socktype = SOCK_DGRAM;
	aiHints.ai_protocol = IPPROTO_UDP;
	if ((retVal = getaddrinfo(szMyName, port, &aiHints, &aiList)) != 0) 
	{
        printf("[PacketClient] getaddrinfo failed. ");
        return 0;
	}

    memcpy(&addr, aiList->ai_addr, aiList->ai_addrlen);
    freeaddrinfo(aiList);
    Addresses[0] = addr.sin_addr.s_addr;
    return 1;
}

bool DecodeTimecode(unsigned int inTimecode, unsigned int inTimecodeSubframe, int* hour, int* minute, int* second, int* frame, int* subframe)
{
	bool bValid = true;

	*hour = (inTimecode>>24)&255;
	*minute = (inTimecode>>16)&255;
	*second = (inTimecode>>8)&255;
	*frame = inTimecode&255;
	*subframe = inTimecodeSubframe;

	return bValid;
}

bool TimecodeStringify(unsigned int inTimecode, unsigned int inTimecodeSubframe, char *Buffer, int BufferSize)
{
	bool bValid;
	int hour, minute, second, frame, subframe;
	bValid = DecodeTimecode(inTimecode, inTimecodeSubframe, &hour, &minute, &second, &frame, &subframe);

	snprintf(Buffer,BufferSize,"%2d:%2d:%2d:%2d.%d",hour, minute, second, frame, subframe);
	for(unsigned int i=0; i<strlen(Buffer); i++)
		if(Buffer[i]==' ')
			Buffer[i]='0';

	return bValid;
}


void Unpack(char* pData)
{
    int major = NatNetVersion[0];
    int minor = NatNetVersion[1];

    char *ptr = pData;

    printf("Begin Packet\n-------\n");

    // message ID
    int MessageID = 0;
    memcpy(&MessageID, ptr, 2); ptr += 2;
    printf("Message ID : %d\n", MessageID);

    // size
    int nBytes = 0;
    memcpy(&nBytes, ptr, 2); ptr += 2;
    printf("Byte count : %d\n", nBytes);
	
    if(MessageID == 7)      // FRAME OF MOCAP DATA packet
    {
        // frame number
        int frameNumber = 0; memcpy(&frameNumber, ptr, 4); ptr += 4;
        printf("Frame # : %d\n", frameNumber);
    	
	    // number of data sets (markersets, rigidbodies, etc)
        int nMarkerSets = 0; memcpy(&nMarkerSets, ptr, 4); ptr += 4;
        printf("Marker Set Count : %d\n", nMarkerSets);

        for (int i=0; i < nMarkerSets; i++)
        {    
            // Markerset name
            char szName[256];
            strcpy(szName, ptr);
            int nDataBytes = (int) strlen(szName) + 1;
            ptr += nDataBytes;
            printf("Model Name: %s\n", szName);

        	// marker data
            int nMarkers = 0; memcpy(&nMarkers, ptr, 4); ptr += 4;
            printf("Marker Count : %d\n", nMarkers);

            for(int j=0; j < nMarkers; j++)
            {
                float x = 0; memcpy(&x, ptr, 4); ptr += 4;
                float y = 0; memcpy(&y, ptr, 4); ptr += 4;
                float z = 0; memcpy(&z, ptr, 4); ptr += 4;
                printf("\tMarker %d : [x=%3.2f,y=%3.2f,z=%3.2f]\n",j,x,y,z);
            }
        }

	    // unidentified markers
        int nOtherMarkers = 0; memcpy(&nOtherMarkers, ptr, 4); ptr += 4;
        printf("Unidentified Marker Count : %d\n", nOtherMarkers);
        for(int j=0; j < nOtherMarkers; j++)
        {
            float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
            float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
            float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
            printf("\tMarker %d : pos = [%3.2f,%3.2f,%3.2f]\n",j,x,y,z);
        }
        
        // rigid bodies
        int nRigidBodies = 0;
        memcpy(&nRigidBodies, ptr, 4); ptr += 4;
        printf("Rigid Body Count : %d\n", nRigidBodies);
        for (int j=0; j < nRigidBodies; j++)
        {
            // rigid body pos/ori
            int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
            float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
            float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
            float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
            float qx = 0; memcpy(&qx, ptr, 4); ptr += 4;
            float qy = 0; memcpy(&qy, ptr, 4); ptr += 4;
            float qz = 0; memcpy(&qz, ptr, 4); ptr += 4;
            float qw = 0; memcpy(&qw, ptr, 4); ptr += 4;
            printf("ID : %d\n", ID);
            printf("pos: [%3.2f,%3.2f,%3.2f]\n", x,y,z);
            printf("ori: [%3.2f,%3.2f,%3.2f,%3.2f]\n", qx,qy,qz,qw);

            // associated marker positions
            int nRigidMarkers = 0;  memcpy(&nRigidMarkers, ptr, 4); ptr += 4;
            printf("Marker Count: %d\n", nRigidMarkers);
            int nBytes = nRigidMarkers*3*sizeof(float);
            float* markerData = (float*)malloc(nBytes);
            memcpy(markerData, ptr, nBytes);
            ptr += nBytes;
            
            if(major >= 2)
            {
                // associated marker IDs
                nBytes = nRigidMarkers*sizeof(int);
                int* markerIDs = (int*)malloc(nBytes);
                memcpy(markerIDs, ptr, nBytes);
                ptr += nBytes;

                // associated marker sizes
                nBytes = nRigidMarkers*sizeof(float);
                float* markerSizes = (float*)malloc(nBytes);
                memcpy(markerSizes, ptr, nBytes);
                ptr += nBytes;

                // 2.6 and later
                if( ((major == 2)&&(minor >= 6)) || (major > 2) || (major == 0) ) 
                {
                    // params
                    short params = 0; memcpy(&params, ptr, 2); ptr += 2;
                    bool bTrackingValid = params & 0x01; // 0x01 : rigid body was successfully tracked in this frame
                }

                for(int k=0; k < nRigidMarkers; k++)
                {
                    printf("\tMarker %d: id=%d\tsize=%3.1f\tpos=[%3.2f,%3.2f,%3.2f]\n", k, markerIDs[k], markerSizes[k], markerData[k*3], markerData[k*3+1],markerData[k*3+2]);
                }

                if(markerIDs)
                    free(markerIDs);
                if(markerSizes)
                    free(markerSizes);

            }
            else
            {
                for(int k=0; k < nRigidMarkers; k++)
                {
                    printf("\tMarker %d: pos = [%3.2f,%3.2f,%3.2f]\n", k, markerData[k*3], markerData[k*3+1],markerData[k*3+2]);
                }
            }
            if(markerData)
                free(markerData);

            if(major >= 2)
            {
                // Mean marker error
                float fError = 0.0f; memcpy(&fError, ptr, 4); ptr += 4;
                printf("Mean marker error: %3.2f\n", fError);
            }

            
	    } // next rigid body


        // skeletons (version 2.1 and later)
        if( ((major == 2)&&(minor>0)) || (major>2))
        {
            int nSkeletons = 0;
            memcpy(&nSkeletons, ptr, 4); ptr += 4;
            printf("Skeleton Count : %d\n", nSkeletons);
            for (int j=0; j < nSkeletons; j++)
            {
                // skeleton id
                int skeletonID = 0;
                memcpy(&skeletonID, ptr, 4); ptr += 4;
                // # of rigid bodies (bones) in skeleton
                int nRigidBodies = 0;
                memcpy(&nRigidBodies, ptr, 4); ptr += 4;
                printf("Rigid Body Count : %d\n", nRigidBodies);
                for (int j=0; j < nRigidBodies; j++)
                {
                    // rigid body pos/ori
                    int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
                    float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
                    float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
                    float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
                    float qx = 0; memcpy(&qx, ptr, 4); ptr += 4;
                    float qy = 0; memcpy(&qy, ptr, 4); ptr += 4;
                    float qz = 0; memcpy(&qz, ptr, 4); ptr += 4;
                    float qw = 0; memcpy(&qw, ptr, 4); ptr += 4;
                    printf("ID : %d\n", ID);
                    printf("pos: [%3.2f,%3.2f,%3.2f]\n", x,y,z);
                    printf("ori: [%3.2f,%3.2f,%3.2f,%3.2f]\n", qx,qy,qz,qw);

                    // associated marker positions
                    int nRigidMarkers = 0;  memcpy(&nRigidMarkers, ptr, 4); ptr += 4;
                    printf("Marker Count: %d\n", nRigidMarkers);
                    int nBytes = nRigidMarkers*3*sizeof(float);
                    float* markerData = (float*)malloc(nBytes);
                    memcpy(markerData, ptr, nBytes);
                    ptr += nBytes;

                    // associated marker IDs
                    nBytes = nRigidMarkers*sizeof(int);
                    int* markerIDs = (int*)malloc(nBytes);
                    memcpy(markerIDs, ptr, nBytes);
                    ptr += nBytes;

                    // associated marker sizes
                    nBytes = nRigidMarkers*sizeof(float);
                    float* markerSizes = (float*)malloc(nBytes);
                    memcpy(markerSizes, ptr, nBytes);
                    ptr += nBytes;

                    for(int k=0; k < nRigidMarkers; k++)
                    {
                        printf("\tMarker %d: id=%d\tsize=%3.1f\tpos=[%3.2f,%3.2f,%3.2f]\n", k, markerIDs[k], markerSizes[k], markerData[k*3], markerData[k*3+1],markerData[k*3+2]);
                    }

                    // Mean marker error
                    float fError = 0.0f; memcpy(&fError, ptr, 4); ptr += 4;
                    printf("Mean marker error: %3.2f\n", fError);

                    // release resources
                    if(markerIDs)
                        free(markerIDs);
                    if(markerSizes)
                        free(markerSizes);
                    if(markerData)
                        free(markerData);

                } // next rigid body

            } // next skeleton
        }
        
		// labeled markers (version 2.3 and later)
		if( ((major == 2)&&(minor>=3)) || (major>2))
		{
			int nLabeledMarkers = 0;
			memcpy(&nLabeledMarkers, ptr, 4); ptr += 4;
			printf("Labeled Marker Count : %d\n", nLabeledMarkers);
			for (int j=0; j < nLabeledMarkers; j++)
			{
				// id
				int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
				// x
				float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
				// y
				float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
				// z
				float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
				// size
				float size = 0.0f; memcpy(&size, ptr, 4); ptr += 4;

                // 2.6 and later
                if( ((major == 2)&&(minor >= 6)) || (major > 2) || (major == 0) ) 
                {
                    // marker params
                    short params = 0; memcpy(&params, ptr, 2); ptr += 2;
                    bool bOccluded = params & 0x01;     // marker was not visible (occluded) in this frame
                    bool bPCSolved = params & 0x02;     // position provided by point cloud solve
                    bool bModelSolved = params & 0x04;  // position provided by model solve
                }

				printf("ID  : %d\n", ID);
				printf("pos : [%3.2f,%3.2f,%3.2f]\n", x,y,z);
				printf("size: [%3.2f]\n", size);
			}
		}

		// latency
        float latency = 0.0f; memcpy(&latency, ptr, 4);	ptr += 4;
        printf("latency : %3.3f\n", latency);

		// timecode
		unsigned int timecode = 0; 	memcpy(&timecode, ptr, 4);	ptr += 4;
		unsigned int timecodeSub = 0; memcpy(&timecodeSub, ptr, 4); ptr += 4;
		char szTimecode[128] = "";
		TimecodeStringify(timecode, timecodeSub, szTimecode, 128);

        // timestamp
        float timestamp = 0.0f;  memcpy(&timestamp, ptr, 4); ptr += 4;

        // frame params
        short params = 0;  memcpy(&params, ptr, 2); ptr += 2;
        bool bIsRecording = params & 0x01;                  // 0x01 Motive is recording
        bool bTrackedModelsChanged = params & 0x02;         // 0x02 Actively tracked model list has changed


		// end of data tag
        int eod = 0; memcpy(&eod, ptr, 4); ptr += 4;
        printf("End Packet\n-------------\n");

    }
    else if(MessageID == 5) // Data Descriptions
    {
        // number of datasets
        int nDatasets = 0; memcpy(&nDatasets, ptr, 4); ptr += 4;
        printf("Dataset Count : %d\n", nDatasets);

        for(int i=0; i < nDatasets; i++)
        {
            printf("Dataset %d\n", i);

            int type = 0; memcpy(&type, ptr, 4); ptr += 4;
            printf("Type : %d  %d\n", i, type);

            if(type == 0)   // markerset
            {
                // name
                char szName[256];
                strcpy(szName, ptr);
                int nDataBytes = (int) strlen(szName) + 1;
                ptr += nDataBytes;
                printf("Markerset Name: %s\n", szName);

        	    // marker data
                int nMarkers = 0; memcpy(&nMarkers, ptr, 4); ptr += 4;
                printf("Marker Count : %d\n", nMarkers);

                for(int j=0; j < nMarkers; j++)
                {
                    char szName[256];
                    strcpy(szName, ptr);
                    int nDataBytes = (int) strlen(szName) + 1;
                    ptr += nDataBytes;
                    printf("Marker Name: %s\n", szName);
                }
            }
            else if(type ==1)   // rigid body
            {
                if(major >= 2)
                {
                    // name
                    char szName[MAX_NAMELENGTH];
                    strcpy(szName, ptr);
                    ptr += strlen(ptr) + 1;
                    printf("Name: %s\n", szName);
                }

                int ID = 0; memcpy(&ID, ptr, 4); ptr +=4;
                printf("ID : %d\n", ID);
             
                int parentID = 0; memcpy(&parentID, ptr, 4); ptr +=4;
                printf("Parent ID : %d\n", parentID);
                
                float xoffset = 0; memcpy(&xoffset, ptr, 4); ptr +=4;
                printf("X Offset : %3.2f\n", xoffset);

                float yoffset = 0; memcpy(&yoffset, ptr, 4); ptr +=4;
                printf("Y Offset : %3.2f\n", yoffset);

                float zoffset = 0; memcpy(&zoffset, ptr, 4); ptr +=4;
                printf("Z Offset : %3.2f\n", zoffset);

            }
            else if(type ==2)   // skeleton
            {
                char szName[MAX_NAMELENGTH];
                strcpy(szName, ptr);
                ptr += strlen(ptr) + 1;
                printf("Name: %s\n", szName);

                int ID = 0; memcpy(&ID, ptr, 4); ptr +=4;
                printf("ID : %d\n", ID);

                int nRigidBodies = 0; memcpy(&nRigidBodies, ptr, 4); ptr +=4;
                printf("RigidBody (Bone) Count : %d\n", nRigidBodies);

                for(int i=0; i< nRigidBodies; i++)
                {
                    if(major >= 2)
                    {
                        // RB name
                        char szName[MAX_NAMELENGTH];
                        strcpy(szName, ptr);
                        ptr += strlen(ptr) + 1;
                        printf("Rigid Body Name: %s\n", szName);
                    }

                    int ID = 0; memcpy(&ID, ptr, 4); ptr +=4;
                    printf("RigidBody ID : %d\n", ID);

                    int parentID = 0; memcpy(&parentID, ptr, 4); ptr +=4;
                    printf("Parent ID : %d\n", parentID);

                    float xoffset = 0; memcpy(&xoffset, ptr, 4); ptr +=4;
                    printf("X Offset : %3.2f\n", xoffset);

                    float yoffset = 0; memcpy(&yoffset, ptr, 4); ptr +=4;
                    printf("Y Offset : %3.2f\n", yoffset);

                    float zoffset = 0; memcpy(&zoffset, ptr, 4); ptr +=4;
                    printf("Z Offset : %3.2f\n", zoffset);
                }
            }

        }   // next dataset

       printf("End Packet\n-------------\n");

    }
    else
    {
        printf("Unrecognized Packet Type.\n");
    }

}
