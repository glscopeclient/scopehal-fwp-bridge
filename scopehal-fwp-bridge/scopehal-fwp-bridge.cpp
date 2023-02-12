#include <ws2tcpip.h>
#include "bridge.h"
#include "BridgeChannel.h"
#include <vector>

bool SendLooped(SOCKET sock, const unsigned char* buf, int count);

using namespace std;

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

	WaveformHeader emptyChannel = { 0 };

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

		bool channelsEnabled[4] = { true, true, true, true };
		while(true)
		{
			//See if a channel enable/disable command came in and process it if so
			WSAPOLLFD poll;
			poll.fd = client;
			poll.events = POLLRDNORM;
			poll.revents = 0;
			if(SOCKET_ERROR == WSAPoll(&poll, 1, 0))
			{
				printf("socket poll failed\n");
				break;
			}
			if(poll.revents & POLLRDNORM)
			{
				unsigned char channels;
				if(1 != recv(client, (char*)&channels, 1, 0))
				{
					printf("socket read failed\n");
					break;
				}

				for(int i = 0; i < 4; i++)
					channelsEnabled[i] = (channels & (1 << i));
			}

			//No commands came in,  but socked was closed
			if(poll.revents & POLLHUP)
			{
				printf("socket disconnected\n");
				break;
			}

			//Make the list of *enabled* channels
			vector<HANDLE> activeChannels;
			for(int i = 0; i < 4; i++)
			{
				if(channelsEnabled[i])
					activeChannels.push_back(handles[i]);
			}
			
			//Wait for all of the enabled channels to have data available
			if(WAIT_TIMEOUT == WaitForMultipleObjects((DWORD)activeChannels.size(), &activeChannels[0], TRUE, 5))
				continue;

			//Send headers for each channel
			bool fail = false;
			for(int i = 0; i < 4; i++)
			{
				//Channel enabled, send actual header
				if(channelsEnabled[i])
				{
					if(!SendLooped(client, (uint8_t*)channels[i].GetHeader(), sizeof(WaveformHeader)))
					{
						printf("fail 1\n");
						fail = true;
					}
				}

				//Channel disabled, send dummy header indicating 'no data'
				else
				{
					if(!SendLooped(client, (uint8_t*)&emptyChannel, sizeof(WaveformHeader)))
					{
						printf("fail 2\n");
						fail = true;
					}
				}
			}

			//Send data for each channel then tell the scope we're finished
			for(int i = 0; i < 4; i++)
			{
				if(channelsEnabled[i])
				{
					if(!SendLooped(client, (uint8_t*)channels[i].GetData(), channels[i].GetHeader()->numSamples * sizeof(int16_t)))
					{
						printf("fail 3\n");
						fail = true;
					}

					//mark output as invalid so the scope doesnt waste time rendering it
					channels[i].GetHeader()->flags = 1;
					channels[i].SetDoneEvent();
				}
			}

			if(fail)
			{
				printf("client disconnected\n");
				break;
			}
		}

		closesocket(client);
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