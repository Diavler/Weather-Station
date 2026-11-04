package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"log"
	"net"
)

type SensorBundle struct {
	Temperature float32
	Pressure    float32
	Humidity    float32
}

func main() {
	addr, err := net.ResolveUDPAddr("udp", ":9000")
	if err != nil {
		log.Fatal("Failed to resolve UDP address:", err)
	}

	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		log.Fatal("Failed to listen:", err)
	}
	defer conn.Close()

	buffer := make([]byte, 1024)
	for {
		n, udpAddr, err := conn.ReadFromUDP(buffer)
		if err != nil {
			log.Printf("Error reading from UDP connection: %v", err)
			continue
		}
		log.Printf("Read %d bytes from %v", n, udpAddr)

		reader := bytes.NewReader(buffer)

		var vec SensorBundle
		err = binary.Read(reader, binary.LittleEndian, &vec)
		if err != nil {
			log.Printf("Error reading sensor bundle: %v", err)
			continue
		}

		fmt.Printf("%.2f C   %.2f kPa   %.2f%% Humdity\n", vec.Temperature, vec.Pressure, vec.Humidity)
	}
}
