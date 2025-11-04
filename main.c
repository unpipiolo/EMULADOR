#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MEM_SIZE 4096

uint16_t PUERTO_SALIDA = 0x0077;

uint16_t memoria[MEM_SIZE];
uint16_t A = 0, B = 0;
uint8_t z = 0;
uint16_t pc = 0;

/* ==== Funciones de memoria ==== */
void mem_write(uint16_t address, uint16_t value) {
    if (address < MEM_SIZE)
        memoria[address] = value;
    else
        printf("Error: Dirección fuera de rango\n");
}

uint16_t mem_read(uint16_t address) {
    if (address < MEM_SIZE)
        return memoria[address];
    else {
        printf("Error: Dirección fuera de rango\n");
        return 0;
    }
}

/* ==== Diccionario de instrucciones ==== */
int obtener_opcode(const char *instr) {
    if (strcmp(instr, "ADD") == 0) return 0b000;
    if (strcmp(instr, "SUB") == 0) return 0b001;
    if (strcmp(instr, "AND") == 0) return 0b010;
    if (strcmp(instr, "ORR") == 0) return 0b011;
    if (strcmp(instr, "RED") == 0) return 0b100;
    if (strcmp(instr, "WRT") == 0) return 0b101;
    if (strcmp(instr, "BEQ") == 0) return 0b110;
    if (strcmp(instr, "HLT") == 0) return 0b111;
    return -1;
}

int registro_valido(char r) {
    return (r == 'A' || r == 'B');
}

/* ==== Carga del programa ==== */
int cargarPrograma(char *ruta) {
    FILE *programa = fopen(ruta, "r");
    if (programa == NULL) {
        perror("Error al abrir el archivo del programa");
        return 1;
    }

    char linea[256];
    int nLinea = 0;
    pc = 0;

    while (fgets(linea, sizeof(linea), programa)) {
        nLinea++;
        linea[strcspn(linea, "\n")] = 0;
        if (strlen(linea) == 0) continue;

        // Línea de datos: formato @direccion valor
        if (linea[0] == '@') {
            char *dir_str = strtok(linea + 1, "\t ");
            char *val_str = strtok(NULL, "\t ");
            if (!dir_str || !val_str) {
                fprintf(stderr, "Error en línea %d: formato de dato inválido\n", nLinea);
                return 1;
            }
            unsigned int dir = strtol(dir_str, NULL, 16);
            unsigned int val = strtol(val_str, NULL, 16);
            memoria[dir] = (uint16_t)val;
            continue;
        }

        // Línea de instrucción
        char *instr = strtok(linea, "\t ");
        char *reg = strtok(NULL, "\t ");
        char *dir = strtok(NULL, "\t ");

        if (!instr) continue;
        int opcode = obtener_opcode(instr);
        if (opcode == -1) {
            fprintf(stderr, "Error en línea %d: instrucción inválida '%s'\n", nLinea, instr);
            return 1;
        }

        uint16_t palabra = 0;
        if (opcode == 0b111) { // HLT
            palabra = (opcode << 13);
            memoria[pc++] = palabra;
            continue;
        }

        if (!reg || !dir) {
            fprintf(stderr, "Error en línea %d: faltan argumentos\n", nLinea);
            return 1;
        }

        if (!registro_valido(reg[0])) {
            fprintf(stderr, "Error en línea %d: registro inválido '%s'\n", nLinea, reg);
            return 1;
        }

        unsigned int direccion = strtol(dir, NULL, 16);
        if (direccion >= MEM_SIZE) {
            fprintf(stderr, "Error en línea %d: dirección fuera de rango (0x%X)\n", nLinea, direccion);
            return 1;
        }

        uint16_t reg_bit = (reg[0] == 'A') ? 0 : 1;
        palabra = (opcode << 13) | (reg_bit << 12) | (direccion & 0x0FFF);
        if (pc >= MEM_SIZE) {
            fprintf(stderr, "Error: memoria llena en línea %d\n", nLinea);
            return 1;
        }
        memoria[pc++] = palabra;
    }

    fclose(programa);
    return 0;
}

/* ==== Instrucciones ==== */
void instr_read(uint8_t reg, uint16_t dir) {
    if (reg) B = memoria[dir], z = (B == 0);
    else A = memoria[dir], z = (A == 0);
}

void instr_add(uint8_t reg, uint16_t dir) {
    if (reg) B += memoria[dir], z = (B == 0);
    else A += memoria[dir], z = (A == 0);
}

void instr_and(uint8_t reg, uint16_t dir) {
    if (reg) B &= memoria[dir], z = (B == 0);
    else A &= memoria[dir], z = (A == 0);
}

void instr_or(uint8_t reg, uint16_t dir) {
    if (reg) B |= memoria[dir], z = (B == 0);
    else A |= memoria[dir], z = (A == 0);
}

void instr_write(uint8_t registro_bit, uint16_t direccion) {
    if (direccion == PUERTO_SALIDA) {
        if (registro_bit == 0) {
            printf("SALIDA (A): %u\n", A);
        } else {
            printf("SALIDA (B): %u\n", B);
        }
    } 
    else {
        if (registro_bit == 0) {
            memoria[direccion] = A;
        } else {
            memoria[direccion] = B;
        }
    }
}


void instr_sub(uint8_t reg, uint16_t dir) {
    if (reg) B -= memoria[dir], z = (B == 0);
    else A -= memoria[dir], z = (A == 0);
}

void instr_beq(uint16_t dir) {
    if (z) pc = dir;
}

/* ==== Bucle principal ==== */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_del_archivo_del_programa>\n", argv[0]);
        return 1;
    }

    if (cargarPrograma(argv[1]) == 1)
        return 1;

    pc = 0;
    int halt = 0;

    while (!halt && pc < MEM_SIZE) {
        uint16_t instr = memoria[pc++];
        uint8_t opcode = (instr >> 13) & 0x7;
        uint8_t reg = (instr >> 12) & 0x1;
        uint16_t dir = instr & 0x0FFF;

        switch (opcode) {
            case 0b000: instr_add(reg, dir); break;
            case 0b001: instr_sub(reg, dir); break;
            case 0b010: instr_and(reg, dir); break;
            case 0b011: instr_or(reg, dir); break;
            case 0b100: instr_read(reg, dir); break;
            case 0b101: instr_write(reg, dir); break;
            case 0b110: instr_beq(dir); break;
            case 0b111: halt = 1; break;
            default:
                printf("Error: opcode desconocido en 0x%03X\n", pc - 1);
                halt = 1;
                break;
        }
    }

    printf("Ejecución finalizada.\nRegistro A = %u, Registro B = %u\n", A, B);
    return 0;
}
