#include "hf_cmd_interpreter.h"
#include "hf_bt_wf_utils.h" // Necesario para acceder a las variables y métodos de Wi-Fi

String process_command(String raw_cmd) {
    // Limpiamos espacios en blanco, saltos de línea o retornos de carro indeseados
    raw_cmd.trim(); 

    // 1. Comando: "show conf"
    if (raw_cmd == "show conf") {
        String status = (WiFi.status() == WL_CONNECTED) ? "Conectado" : "Desconectado";
        String ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "0.0.0.0";
        
        String res = "\n--- CONFIGURACION ACTUAL ---\n";
        res += "SSID: " + (wf_ssid == "" ? "[Vacio]" : wf_ssid) + "\n";
        res += "PASS: " + (wf_pwd == "" ? "[Vacio]" : wf_pwd) + "\n";
        res += "Estado: " + status + "\n";
        res += "IP: " + ip + "\n";
        return res;
    }

    // 2. Comando: "show times"
    if (raw_cmd == "show times") {
        return "No schedule found";
    }

    // 3. Comando: "set wifi ssid $name_of_ssid"
    if (raw_cmd.startsWith("set wifi ssid ")) {
        // Extraemos todo lo que esté después del carácter 14 ("set wifi ssid ")
        String new_ssid = raw_cmd.substring(14);
        new_ssid.trim();
        
        if (new_ssid.length() == 0) {
            return "Error: El nombre de la SSID no puede estar vacio.";
        }
        
        // Guardamos e intentamos conectar (este método actualiza wf_ssid internamente)
        write_to_store(new_ssid, wf_pwd);
        return "SSID actualizada a '" + new_ssid + "'. Intentando conectar...";
    }

    // 4. Comando: "set wifi pass $wifi_password"
    if (raw_cmd.startsWith("set wifi pass ")) {
        // Extraemos todo lo que esté después del carácter 14 ("set wifi pass ")
        String new_pass = raw_cmd.substring(14);
        new_pass.trim();
        
        // Guardamos e intentamos conectar (este método actualiza wf_pwd internamente)
        write_to_store(wf_ssid, new_pass);
        return "Password actualizada. Intentando conectar...";
    }

    // 5. Comando: "help"
    if (raw_cmd == "help") {
        String h = "\n--- COMANDOS DISPONIBLES ---\n";
        h += "- show conf: Muestra la configuracion de la wifi (ssid y password), estado de conexion e IP.\n";
        h += "- show times: Muestra las horas de inicio y parada registradas.\n";
        h += "- set wifi ssid $name_of_ssid: Establece el nombre de la red Wi-Fi y reconecta.\n";
        h += "- set wifi pass $wifi_password: Establece la contrasena de la red Wi-Fi y reconecta.\n";
        h += "- help: Muestra esta guia de comandos.\n";
        return h;
    }

    // 6. Cualquier otro comando no reconocido
    return "Command not found. Send 'help' to list available commands";
}