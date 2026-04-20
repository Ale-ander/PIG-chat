package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"path"
)

type Config struct {
	IpTuo     string `json:"tuo"`
	ChiratuIp string `json:"chiratu"`
}

func loadConfig(pigConfigPath string) Config {
	if _, errPath := os.Stat(pigConfigPath); errPath != nil && os.IsNotExist(errPath) {
		if errCreateDir := os.Mkdir(pigConfigPath, 0755); errCreateDir != nil {
			log.Fatalf("Nun aggiu creato la directory %q: %q", pigConfigPath, errCreateDir.Error())
		}
	}

	configPath := path.Join(pigConfigPath, "config.json")
	_, errInfo := os.Lstat(configPath)
	if errInfo != nil {
		if os.IsNotExist(errInfo) {
			cfg := Config{
				IpTuo:     "mittici l'IP tuo",
				ChiratuIp: "mittici l'IP re chiratu",
			}

			serialized, errSerialize := json.Marshal(cfg)
			if errSerialize != nil {
				log.Fatal("Nun aggiu riuscito a serializzá la config: ", errSerialize)
			}
			errWrite := os.WriteFile(configPath, serialized, 0644)
			if errWrite != nil {
				log.Fatal("Nun aggiu riuscito a scrive la config: ", errWrite)
			}
			fmt.Printf("Nun c'era la config, te naggio fatta una re esempio a %q, valla a modifica subbutu\n", pigConfigPath)
			os.Exit(0)
		} else {
			log.Fatal("Nge nu problema: %q", errInfo.Error())
		}
	}

	file, errOpenFile := os.Open(configPath)
	if errOpenFile != nil {
		log.Fatal("Nge nu problema: %q", errOpenFile.Error())
	}
	defer file.Close()

	cfgAsBytes, errRead := io.ReadAll(file)
	if errRead != nil {
		log.Fatal("Nge nu problema: %q", errRead.Error())
	}

	var cfg Config
	json.Unmarshal(cfgAsBytes, &cfg)

	return cfg
}

var (
	msg string
	h bool
)

func main() {
	// setup
	homeDir, errHome := os.UserHomeDir()
	if errHome != nil {
		log.Fatalf("Non riesco a trovare il porcile, indirizzo del porcile %q: %q", homeDir, errHome.Error())
	}

	pigChatConfigDir := path.Join(homeDir, ".config", "pigchat")
	config := loadConfig(pigChatConfigDir)

	var (
		msg string
		ip string
		h bool
	)
	flag.StringVar(&msg, "m", "se non scrivi nu messaggio si nu puorcu", "message")
	flag.BoolVar(&h, "h", false, "help")
	flag.StringVar(&ip, "ip", config.ChiratuIp, "ip address to which send the grunt 🐷")

	flag.Parse()

	if h || msg ==  "se non scrivi nu messaggio si nu puorcu" {
		flag.Usage()
		return
	}

	buf := bytes.NewBufferString(msg)
	url := fmt.Sprintf("http://%s/recv", ip)
	resp, errHttp := http.Post(url, msg, buf)

	if errHttp != nil {
		log.Fatalf("Lu grugnito http se scassato ra qualche parte: %q", errHttp.Error())
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		fmt.Printf("La risposta nun è 200 ok, viritilla tu: %q", resp.Status)
	}
}
