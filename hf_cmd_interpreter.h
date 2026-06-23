#ifndef HF_CMD_INTERPRETER_H
#define HF_CMD_INTERPRETER_H

#include <Arduino.h>

// Función principal que recibe el comando crudo y devuelve la respuesta procesada
String process_command(String raw_cmd);

#endif