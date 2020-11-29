/*
 * echo_server_v0.rs
 * 
 * A simple echo server which can handle only connection
 * at a time. Uses blocking calls.
 * 
 * Equivalent to echo_server_v0.c
 */

use std::{env, io, net};
use std::io::{Read, Write};

fn main () -> io::Result<()>
{
    // Get the arguments
    let args: Vec<String> = env::args().collect();
    if args.len() != 3
    {
        println!("Usage: {} [ipv4 address] [port number]", args[0]);
        return Ok(())
    }

    // Generate the address tuple
    let ip_addr = &args[1];
    let port_no = &args[2];
    let mut address = String::new();
    address.push_str(ip_addr.as_str());
    address.push_str(":");
    address.push_str(port_no.as_str());

    // Create the listener
    let listener = net::TcpListener::bind(&address)?;
    println!("Listening at {}", address);

    // Let us start the server
    loop
    {
        match listener.accept()
        {
            Ok((client_stream, client_addr)) =>
            {
                println!("Connection from {:?} accepted", client_addr);
                serve_connection(client_stream)?;
            }
            Err(error) =>
            {
                println!("Error: {:?}", error);
                return Ok(());
            }
        }
    }
}

fn serve_connection (mut client_stream: net::TcpStream) -> std::result::Result<(), std::io::Error>
{   
    // We have a 10,000 buffer
    let mut request_buffer: [u8; 10000] = [0; 10000];

    // In a loop, we recv data and send back.
    loop
    {
        // Receive data
        let read_bytes_num = client_stream.read(&mut request_buffer)?;
        if read_bytes_num == 0 
        {
            // The client has closed the connection
            println!("Returning back from server_connection");
            return Ok(());
        }
        
        // Send it
        client_stream.write(&request_buffer[..read_bytes_num])?;

        // Clean up the buffer
        request_buffer = [0; 10000];
    }   
}