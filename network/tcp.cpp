#include "network/tcp.h"
#include "Logs.h"
#include "sn_utils.h"

network::tcp_conn::tcp_conn(asio::io_context &io_ctx)
    : m_socket(io_ctx)
{
	m_header_buffer.resize(8);
}

void network::tcp_conn::disconnect()
{
	WriteLog("network: client dropped", logtype::net);
	m_socket.close();
}

void network::tcp_conn::connected()
{
	connection::connected();
	read_header();
}

void network::tcp_conn::read_header()
{
	asio::async_read(m_socket, asio::buffer(m_header_buffer),
	                    std::bind(&tcp_conn::handle_header, this,
	                              std::placeholders::_1, std::placeholders::_2));
}

void network::tcp_conn::handle_send(std::shared_ptr<std::string> buf, const asio::error_code &err, size_t bytes_transferred)
{

}

void network::tcp_conn::send_data(std::shared_ptr<std::string> buffer)
{
	asio::async_write(m_socket, asio::buffer(*buffer.get()), std::bind(&tcp_conn::handle_send, this, buffer,
	                                                            std::placeholders::_1,
	                                                            std::placeholders::_2));
}

void network::tcp_conn::handle_header(const asio::error_code &err, size_t bytes_transferred)
{
	std::istringstream header(m_header_buffer);
	if (m_header_buffer.size() != bytes_transferred) {
		disconnect();
		return;
	}

	uint32_t sig = sn_utils::ld_uint32(header);
	if (sig != 0x37305545) {
		disconnect();
		return;
	}

	uint32_t len = sn_utils::ld_uint32(header);
	m_body_buffer.resize(len);
	if (len > 10000) {
		disconnect();
		return;
	}

	asio::async_read(m_socket, asio::buffer(m_body_buffer),
	                 std::bind(&tcp_conn::handle_data, this, std::placeholders::_1, std::placeholders::_2));
}

void network::tcp_conn::handle_data(const asio::error_code &err, size_t bytes_transferred)
{
	if (m_body_buffer.size() != bytes_transferred) {
		disconnect();
		return;
	}
	data_received(m_body_buffer);
	read_header();
}

asio::ip::tcp::socket& network::tcp_conn::socket()
{
	return m_socket;
}

network::tcp_server::tcp_server(asio::io_context &io_ctx)
    : m_acceptor(io_ctx)
{
	auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v6(), 7424);
	m_acceptor.open(endpoint.protocol());
	m_acceptor.set_option(asio::socket_base::reuse_address(true));
	m_acceptor.set_option(asio::ip::v6_only(false));
	m_acceptor.bind(endpoint);
	m_acceptor.listen(10);

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

network::tcp_client::tcp_client(asio::io_context &io_ctx)
{
	conn = std::make_shared<tcp_conn>(io_ctx);
	auto tcpconn = std::static_pointer_cast<tcp_conn>(conn);

	asio::ip::tcp::endpoint endpoint(
	            asio::ip::address::from_string("127.0.0.1"), 7424);
	tcpconn->socket().async_connect(endpoint,
	                    std::bind(&tcp_client::handle_accept, this, std::placeholders::_1));
}

void network::tcp_client::handle_accept(const asio::error_code &err)
{
	if (!err)
	{
		conn->connected();
	}
	else
	{
		WriteLog(std::string("net: failed to connect: " + err.message()), logtype::net);
	}
}
