#pragma once

#include <asio.hpp>
#include <memory>
#include <array>
#include "network/network.h"

namespace network
{
    class tcp_conn
	        : public connection
	{
	private:
		asio::ip::tcp::socket m_socket;

		std::string m_header_buffer;
		std::string m_body_buffer;

		void read_header();
		void handle_send(std::shared_ptr<std::string> buf, const asio::error_code &err, size_t bytes_transferred);
		void handle_header(const asio::error_code &err, size_t bytes_transferred);
		void handle_data(const asio::error_code &err, size_t bytes_transferred);

	protected:
		void disconnect() override;
		void send_data(std::shared_ptr<std::string> buffer) override;

	public:
		tcp_conn(asio::io_context &io_ctx);
		asio::ip::tcp::socket& socket();

		void connected() override;
	};

	class tcp_server : public server
	{
	public:
		tcp_server(asio::io_context &io_ctx);

	private:
		void accept_conn();
		void handle_accept(std::shared_ptr<tcp_conn> conn, const asio::error_code &err);

		asio::ip::tcp::acceptor m_acceptor;
	};
}
