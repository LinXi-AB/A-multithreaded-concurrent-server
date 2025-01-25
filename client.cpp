#include <arpa/inet.h>   // 提供 sockaddr_in、inet_pton() 等网络相关功能
#include <cstring>      
#include <iostream>     
#include <sys/socket.h>  // 提供 socket()、connect()、send() 和 recv() 等函数
#include <unistd.h>      // 提供 close() 函数

class TCPClient
{
private:
	int clientSocket;
	struct sockaddr_in serverAddr;

public:
	TCPClient();
	~TCPClient();

	bool connectToServer(const std::string& ipAddress, int port);
	bool sendMessage(const std::string& message);
	std::string receiveMessage();
	void closeConnection();
};

TCPClient::TCPClient() : clientSocket(-1)
{
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET; // 使用 IPv4
}

TCPClient::~TCPClient()
{
	closeConnection();
}

// 连接到服务器
bool TCPClient::connectToServer(const std::string& ipAddress, int port)
{
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == -1)
	{
		perror("Socket creation failed");
		return false;
	}

	serverAddr.sin_port = htons(port); // 设置端口
	if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) <= 0)
	{
		perror("Invalid address or address not supported");
		return false;
	}

	// 连接服务器
	if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("Connection to server failed, 请先启动服务端");
		return false;
	}
	return true;
}

// 发送消息
bool TCPClient::sendMessage(const std::string& message)
{
	if (send(clientSocket, message.c_str(), message.size(), 0) < 0)
	{
		perror("Failed to send message");
		return false;
	}
	return true;
}

// 接收消息
std::string TCPClient::receiveMessage()
{
	char buffer[1024] = { 0 }; // 定义缓冲区存储接收的数据
	ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead < 0)
	{
		perror("Failed to receive message");
		return "";
	}

	buffer[bytesRead] = '\0'; // 确保字符串以 '\0' 结尾
	return std::string(buffer);
}

// 关闭连接
void TCPClient::closeConnection()
{
	if (clientSocket >= 0)
	{
		close(clientSocket);
		clientSocket = -1;
	}
}

/**
 * @brief 使用流程：
 * connectToServer(IP, 端口): socket() connect()
 * sendMessage(字符串)
 * reveiveMessage()
 * closeConnection()
 */
int main()
{
	TCPClient client;

	std::string ip_addr("172.30.129.33");
	const int port = 8080;
	if (!client.connectToServer(ip_addr, port))// 测试的时候可以使用127.0.0.1
	{
		return -1;
	}

	if (!client.sendMessage("\"我是客户端\""))
	{
		return -1;
	}

	std::string response = client.receiveMessage();
	if (!response.empty())
	{
		std::cout << "接收到来自服务器 (IP: " << ip_addr
			<< ", 端口: " << port << ") 的数据: " << response << std::endl;
	}

	client.closeConnection();
	return 0;
}
