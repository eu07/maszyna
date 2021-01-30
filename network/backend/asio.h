#ifdef DBG_NEW
#undef new
#endif

#define ASIO_DISABLE_VISIBILITY
#include <asio.hpp>

#include "network/network.h"

namespace network::tcp
{
    const uint32_t NETWORK_MAGIC = 0x37305545;
	const uint32_t MAX_MSG_SIZE = 100000;

	class connection : public network::connection
	{
		friend class server;
		friend class client;

	public:
                connection(asio::io_context &io_ctx, bool client = false, size_t counter = 0);
		~connection();

		virtual void connected() override;
		virtual void disconnect() override;
		virtual void send_messages(const std::vector<std::shared_ptr<message> > &messages) override;
		virtual void send_message(const message &msg) override;

		asio::ip::tcp::socket m_socket;

	private:
		std::string m_header_buffer;
		std::string m_body_buffer;

		void write_message(const message &msg, std::ostream &stream);
		void send_data(std::shared_ptr<std::string> buffer);
		void read_header();
		void handle_header(const asio::error_code &err, size_t bytes_transferred);
		void handle_data(const asio::error_code &err, size_t bytes_transferred);
	};

	class server : public network::server
	{
	private:
		void accept_conn();
		void handle_accept(std::shared_ptr<connection> conn, const asio::error_code &err);

		asio::ip::tcp::acceptor m_acceptor;
                asio::io_context &m_io_ctx;

	public:
		server(std::shared_ptr<std::istream> buf, asio::io_context &io_ctx, const std::string &host, uint32_t port);
	};

	class client : public network::client
	{
	private:
		void handle_accept(const asio::error_code &err);
		asio::io_context &io_ctx;
		std::string host;
		uint32_t port;

	protected:
		virtual void connect() override;

	public:
		client(asio::io_context &io_ctx, const std::string &host, uint32_t port);
	};

	class asio_manager : public network::backend_manager {
		asio::io_context io_context;

	public:
		asio_manager();

		virtual std::shared_ptr<network::server> create_server(std::shared_ptr<std::fstream>, const std::string &conf) override;
		virtual std::shared_ptr<network::client> create_client(const std::string &conf) override;
		virtual void update() override;
	};

	asio_manager manager;
}
