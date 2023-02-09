#include <ws2tcpip.h>
#include "bridge.h"
#include "BridgeChannel.h"

bool SendLooped(SOCKET sock, const unsigned char* buf, int count);

int main(int argc, char* argv[])
{
	const int portnum = 1862;

	BridgeChannel channels[4] = { 1, 2, 3, 4 };

	//Get all of the handles
	HANDLE handles[4] =
	{
		channels[0].GetDataAvailableEvent(),
		channels[1].GetDataAvailableEvent(),
		channels[2].GetDataAvailableEvent(),
		channels[3].GetDataAvailableEvent()
	};

	//Initialize our socket
	WSADATA data;
	if(0 != WSAStartup(MAKEWORD(2, 2), &data))
	{
		printf("WSAStartup failed\n");
		abort();
	}
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock == INVALID_SOCKET)
	{
		printf("socket creation failed\n");
		abort();
	}
	sockaddr_in name;
	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(portnum);
	if(0 != ::bind(sock, reinterpret_cast<sockaddr*>(&name), sizeof(name)))
	{
		printf("socket bind failed\n");
		abort();
	}
	if(0 != listen(sock, SOMAXCONN))
	{
		printf("socket listen failed\n");
		abort();
	}

	//Main loop, waiting for connections
	printf("ready\n");
	while(true)
	{
		sockaddr_in caddr;
		int clen = sizeof(caddr);
		SOCKET client = accept(sock, reinterpret_cast<sockaddr*>(&caddr), &clen);
		if(client == INVALID_SOCKET)
		{
			printf("socket accept failed\n");
			abort();
		}
		printf("Client connected\n");

		//Disable Nagle
		int flag = 1;
		if(0 != setsockopt((int)client, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)))
			return false;

		while(true)
		{
			//TODO: accept control plane commands to turn channels on/off? for now, use all of them all the time

			//Wait for all of the channel to have data available
			WaitForMultipleObjects(4, handles, TRUE, INFINITE);
			printf("signaled\n");

			//TODO: send header indicating how many channels are present

			//Send headers for each channel
			bool fail = false;
			for(int i = 0; i < 4; i++)
			{
				if(!SendLooped(client, (uint8_t*)channels[i].GetHeader(), sizeof(WaveformHeader)))
					fail = true;
			}

			//Send data for each channel
			for(int i = 0; i < 4; i++)
			{
				if(!SendLooped(client, (uint8_t*)channels[i].GetData(), channels[i].GetHeader()->numSamples * sizeof(int16_t)))
					fail = true;
			}

			//Tell the client we're done
			for(int i = 0; i < 4; i++)
				channels[i].SetDoneEvent();

			if(fail)
			{
				printf("client disconnected\n");
				closesocket(client);
				break;
			}
		}

	}
}

bool SendLooped(SOCKET sock, const unsigned char* buf, int count)
{
	const unsigned char* p = buf;
	int bytes_left = count;
	int x = 0;

	while((x = send(sock, (const char*)p, bytes_left, 0)) > 0)
	{
		bytes_left -= x;
		p += x;
		if(bytes_left == 0)
			break;
	}

	if(x <= 0)
		return false;
	return bytes_left == 0;
}