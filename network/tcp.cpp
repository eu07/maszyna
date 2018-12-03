#include "network/tcp.h"
#include "Logs.h"
#include "sn_utils.h"

network::tcp_conn::tcp_conn(asio::io_context &io_ctx)
    : m_socket(io_ctx)
{
	m_header_buffer.resize(12);
}

void network::tcp_conn::disconnect()
{
	m_socket.close();
}

void network::tcp_conn::connected()
{
	read_header();
}

void network::tcp_conn::read_header()
{
	asio::async_read(m_socket, asio::buffer(m_header_buffer),
	                    std::bind(&tcp_conn::handle_header, this, std::placeholders::_1));
}

void network::tcp_conn::send_data(uint8_t *buffer, size_t len)
{

}

void network::tcp_conn::handle_header(const asio::error_code &err)
{
	std::istringstream header(m_header_buffer);
	uint32_t sig = sn_utils::ld_uint32(header);
	if (sig != 0x37305545)
	{
		disconnect();
		return;
	}

	uint32_t len = sn_utils::ld_uint32(header);
	m_body_buffer.resize(len);
	if (len > 10000)
	{
		disconnect();
		return;
	}

	asio::async_read(m_socket, asio::buffer(m_body_buffer),
	                 std::bind(&tcp_conn::handle_data, this, std::placeholders::_1));
}

void network::tcp_conn::handle_data(const asio::error_code &err)
{
	std::istringstream body(m_body_buffer);
	message msg = message::deserialize(body);

	if (msg.type == message::TYPE_MAX)
	{
		disconnect();
		return;
	}

	if (msg.type == message::CONNECT_REQUEST)
	{
		message reply({message::CONNECT_ACCEPT});

		std::string reply_buf;
		std::ostringstream stream(reply_buf);
		reply.serialize(stream);
	}

	read_header();
}

asio::ip::tcp::socket& network::tcp_conn::socket()
{
	return m_socket;
}

network::tcp_server::tcp_server(asio::io_context &io_ctx)
    : m_acceptor(io_ctx, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), 7424))
{
	asio::ip::v6_only v6_only(false);
	m_acceptor.set_option(v6_only);

	accept_conn();
}

void network::tcp_server::accept_conn()
{
	std::shared_ptr<tcp_conn> conn = std::make_shared<tcp_conn>(m_acceptor.get_executor().context());

	m_acceptor.async_accept(conn->socket(), std::bind(&tcp_server::handle_accept, this, conn, std::placeholders::_1));
}

void network::tcp_server::handle_accept(std::shared_ptr<tcp_conn> conn, const asio::error_code &err)
{
	if (!err)
	{
		clients.emplace_back(conn);
		conn->connected();
	}
	else
	{
		WriteLog(std::string("net: failed to accept client: " + err.message()), logtype::net);
	}

	accept_conn();
}
