<?php

class HftConnector
{
    const host = "127.0.0.1";
    private $socket;

    public function __construct($port = 8137) {
        $this -> socket = socket_create(AF_INET, SOCK_STREAM, 0);

        if ($this -> socket === FALSE) {
            throw new Exception('HftConnector: '.socket_strerror(socket_last_error()));
        }

        $result = FALSE;
        for ($i=0; $i<10; $i++) {
            $result = socket_connect($this -> socket, HftConnector::host, $port);
            if ($result !== FALSE) {
                break;
            } else {
                sleep(1);
            }
        }

        if ($result === FALSE) {
            throw new Exception('HftConnector: '.socket_strerror(socket_last_error()));
        }
    }

    public function __destruct() {
        socket_close($this -> socket);
    }

    public function send_message($data) {
        if ($data[strlen($data)-1] != "\n") {
            $data = $data . chr(10);
        }

        $result = socket_write($this -> socket, $data, strlen($data));

        if ($result === FALSE) {
            throw new Exception('HftConnector: '.socket_strerror(socket_last_error()));
        }

        $result = socket_read($this -> socket, 1024);

        if ($result === FALSE) {
            throw new Exception('HftConnector: '.socket_strerror(socket_last_error()));
        }

        return trim($result);
    }
}