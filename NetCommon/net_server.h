#pragma once

#include "net_common.h"
#include "net_thread_safe_queue.h"
#include "net_message.h"
#include "net_connection.h"

namespace olc
{
	namespace net
	{
		template<typename T>
		class server_interface
		{
		public:
			server_interface(uint16_t port)
				: m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
			{}

			virtual ~server_interface()
			{
				// If the server is destroyed, always try and disconnect
				Stop();
			}

			bool Start()
			{
				try
				{
					// Ordering here is important

					// Issue a task to the asio context - This is important
					WaitForClientConnection();

					// Start the context by calling "run" in the thread of its own
					m_threadContext = std::thread([this]() { m_asioContext.run(); });

				}
				catch (std::exception& e)
				{
					// Something prohibited the server from listening
					std::cerr << "[SERVER] Exception: " << e.what() << "\n";
					return false;
				}

				std::cout << "[SERVER] Started!\n";
				return true;
			}

			bool Stop()
			{
				// Request the context to close
				m_asioContext.stop();

				// Tidy up the context thread
				if (m_threadContext.joinable()) m_threadContext.join();

				// Inform someone, anybody, if they care...
				std::cout << "[SERVER] Stopped!\n";

				return true;
			}

			// ASYNC - Instruct asio to wait for connection
			void WaitForClientConnection()
			{
				m_asioAcceptor.async_accept(
					[this](std::error_code ec, asio::ip::tcp::socket socket)
					{
						if (!ec)
						{
							std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

							// Temporarily create a new connection to handle this client
							// Shared_ptr 
							std::shared_ptr<connection<T>> newconn =
								std::make_shared<connection<T>>
								(
									connection<T>::owner::server,	// Fundamentally owned by a server
									m_asioContext,					// Pass in the current asio context
									std::move(socket),				// Socket provided by the async accept function
									m_qMessagesIn					// Pass by reference the incoming queue of the server
																	// Note: only 1 instance of this queue
																	// Note: The queue will be shared across all the connection
								);

							// Give the user server a chance to deny connection
							if (OnClientConnect(newconn))
							{
								// Connection allowed, so add to container of new connections
								m_deqConnections.push_back(std::move(newconn));
								// Valid connections will need to be assigned their ID
								m_deqConnections.back()->ConnectToClient(nIDCounter++);

								std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";
							}
							else
							{
								std::cout << "[------] Connection denied!\n";
							}
						}
						else
						{
							// Error has occured during acceptance
							std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
						}

						// Prime the asio context with more work - again simply wait for another connection...
						WaitForClientConnection();
					}
				);
			}

			// Send a message to a specific client
			void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
			{
				// Check client's shared ptr's validity before sending message
				if (client && client->IsConnected())
				{
					client->Send(msg);
				}
				else
				{
					// Allow user's server side code to remove the client from the game world
					OnClientDisconnected(client);
					client.reset();
					m_deqConnections.erase
					(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), client),
						m_deqConnections.end()
					);
				}
			}

			// Send message to all clients
			// skip_client is optional, and allows us to not send message to a specific client
			void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
			{
				bool bInvalidClientExists = false;

				for (auto& client : m_deqConnections)
				{
					// Check client is connected
					if (client && client->IsConnected())
					{
						if (client != pIgnoreClient)
							client->send(msg);
					}
					else
					{
						OnClientDisconnect(client);
						client.reset();
					}
				}

				if (bInvalidClientExists)
					m_deqConnections.erase
					(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr),
						m_deqConnections.end()
					);
			}

			void Update(size_t nMaxMessages = -1)
			{
				size_t nMessageCount = 0;
				while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
				{
					// Grab the front message
					auto msg = m_qMessagesIn.pop_front();

					// Pass to message handler
					OnMessage(msg.remote, msg.msg);

					nMessageCount++;
				}
			}

		protected:
			// Called when a client connects, you can veto the connection by returning false
			virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
			{
				return false;
			}

			// Called when a client appears to have disconnected
			virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
			{}

			// Called when a message arrives
			virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg)
			{}

			// Thread Safe Queue for incoming message packets
			tsqueue<owned_message<T>> m_qMessagesIn;

			// Container of active validated connections
			std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

			// Order of declaration is important - it is also the order of initialisation
			asio::io_context m_asioContext;
			std::thread m_threadContext;

			// We need sockets of the connecting clients
			// These things need an asio context
			asio::ip::tcp::acceptor m_asioAcceptor; // Handles new incoming connection attempts...

			// Clients will be identified in the "wider system" via an ID
			uint32_t nIDCounter = 10000;
		};
	}
}