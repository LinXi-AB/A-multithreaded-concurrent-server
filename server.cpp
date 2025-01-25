#include <arpa/inet.h> // 提供 sockaddr_in、htons() 等网络相关功能
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define BUFFER_SIZE 1024

class ThreadPool
{
public:
	ThreadPool(size_t thread_count);
	~ThreadPool();
	void addTask(const std::function<void()>& task);

private:
	void worker();

	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks_queue;
	std::mutex queue_mutex;
	std::condition_variable condition;
	std::atomic<bool> stop = false;
};

ThreadPool::ThreadPool(size_t thread_count)
{
	for (size_t i = 0; i < thread_count; ++i)
	{
		threads.emplace_back(&ThreadPool::worker, this);
	}
}

ThreadPool::~ThreadPool()
{
	stop.store(true);
	condition.notify_all();
	for (auto& thread : threads)
	{
		if (thread.joinable())
			thread.join();
	}
}

void ThreadPool::addTask(const std::function<void()>& task)
{
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		tasks_queue.push(task);
	}
	condition.notify_one();
}

void ThreadPool::worker()
{
	while (true)
	{
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			condition.wait(lock, [this]
				{
					return !tasks_queue.empty() || stop;
				});

			if (stop && tasks_queue.empty())
				return;

			task = std::move(tasks_queue.front());
			tasks_queue.pop();
		}
		task();
	}
}

class TCPServer
{
private:
	int server_fd;
	struct sockaddr_in address;
	int addrlen;

public:
	TCPServer(int port);
	~TCPServer();
	bool initialize();
	int acceptClient();
	void closeServer();
};

TCPServer::TCPServer(int port) : server_fd(-1), addrlen(sizeof(address))
{
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
}

TCPServer::~TCPServer()
{
	closeServer();
}

bool TCPServer::initialize()
{
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == 0)
	{
		perror("Socket creation failed");
		return false;
	}

	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) != 0)
	{
		perror("Setsockopt failed");
		close(server_fd);
		return false;
	}

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
	{
		perror("Bind failed");
		close(server_fd);
		return false;
	}

	if (listen(server_fd, 3) < 0)
	{
		perror("Listen failed");
		close(server_fd);
		return false;
	}
	return true;
}

int TCPServer::acceptClient()
{
	int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
	if (client_socket < 0)
	{
		perror("Accept failed");
		return -1;
	}
	return client_socket;
}

void TCPServer::closeServer()
{
	if (server_fd >= 0)
	{
		close(server_fd);
		server_fd = -1;
	}
}

void handleClient(int client_socket)
{
	static std::atomic<int> message_counter(0);  // 消息编号计数器

	char buffer[BUFFER_SIZE] = { 0 };
	int bytesRead = read(client_socket, buffer, BUFFER_SIZE);

	if (bytesRead > 0)
	{
		// 获取消息编号
		int message_id = ++message_counter;

		// 输出接收到的原始数据和消息编号
		std::cout << "(#" << message_id << ") Received message from client: " << std::string(buffer, bytesRead) << std::endl;

		// 输出客户端的 IP 地址和端口
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		getpeername(client_socket, (struct sockaddr*)&client_addr, &client_len);
		std::cout << "Client IP: " << inet_ntoa(client_addr.sin_addr) << std::endl;
		std::cout << "Client port: " << ntohs(client_addr.sin_port) << std::endl;

		// 返回给客户端的数据，包含消息编号
		std::string response = "Message #" + std::to_string(message_id) + " - Server response: " + std::string(buffer, bytesRead);
		send(client_socket, response.c_str(), response.size(), 0);

		// 输出服务器响应内容
		std::cout << "Sent response to client: " << response << std::endl;
	}
	else
	{
		std::cerr << "Failed to read data from client" << std::endl;
	}

	close(client_socket);
}


int main()
{
	const int port = 8080;
	TCPServer server(port);

	if (!server.initialize())
	{
		return -1;
	}

	ThreadPool pool(4);

	std::cout << "Server is running on port " << port << "..." << std::endl;

	while (true)// 服务器主循环
	{
		int client_socket = server.acceptClient();// 阻塞等待一个客户端的链接
		if (client_socket >= 0)// 连接成功则分发任务给线程
		{
			pool.addTask([client_socket]
				{
					handleClient(client_socket);
				});
		}
	}

	server.closeServer();
	return 0;
}
