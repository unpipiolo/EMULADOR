#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint16_t memoria[MEM_SIZE]; // crea ela array
uint16_t A = 0, B = 0;
uint8_t z = 0;
uint16_t pc = 0;

/*
Escribe un valor en la memoria
/
void mem_write(uint16_t address, uint16_t value) {
    // comprueba que la direccion este en rango valido
    if (address < MEM_SIZE) { 
        mem[address] = value; // escribe el valor en la posicion
    } else {
        printf("Error: Dirección fuera de rango\n");
    }

}

/
Lee el valor guardado en la posicion address
*/
uint16_t mem_read(uint16_t address) {
    // comprueba que la direccion este en rango valido
    if (address < MEM_SIZE){ 
        return memoria[address]; // devuelve el valor almacendo
    } else {
        printf("Error: Dirección fuera de rango\n");
        return 0;
    }
}

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

int cargarPrograma(char *ruta) {
    FILE *programa = fopen(ruta, "r");
    if (programa == NULL) {
        perror("Error al abrir el archivo del programa.");
        return 1;
    }

    char linea[256];
    int nLinea = 0;
    int pc = 0;
    while (fgets(linea, sizeof(linea), programa)) {
        nLinea++;
        linea[strcspn(linea, "\n")] = 0;
        if (strlen(linea) == 0) continue;

        // Parsear campos
        char *instr = strtok(linea, "\t");
        char *reg = strtok(NULL, "\t");
        char *dir = strtok(NULL, "\t");

        if (!instr) continue;

        int opcode = obtener_opcode(instr);
        if (opcode == -1) {
            fprintf(stderr, "Error en línea %d: instrucción inválida '%s'\n", nLinea, instr);
            return 1;
        }

        uint16_t palabra = 0;

        // Caso especial: HLT no tiene operandos
        if (opcode == 0b111) {
            palabra = (opcode << 13);
            memoria[pc++] = palabra;
            continue;
        }

        // Validar campos
        if (!reg || !dir) {
            fprintf(stderr, "Error en línea %d: faltan argumentos\n", nLinea);
            return 1;
        }

        if (!registro_valido(reg[0])) {
            fprintf(stderr, "Error en línea %d: registro inválido '%s'\n", nLinea, reg);
            return 1;
        }

        unsigned int direccion = (unsigned int)strtol(dir, NULL, 16);
        if (direccion >= MEM_SIZE) {
            fprintf(stderr, "Error en línea %d: dirección fuera de rango (0x%X)\n", nLinea, direccion);
            return 1;
        }

        uint16_t reg_bit = (reg[0] == 'A') ? 0 : 1;

        // Codificar: [15:13]=opcode | [12]=reg | [11:0]=direccion
        palabra = (opcode << 13) | (reg_bit << 12) | (direccion & 0x0FFF);

        if (pc >= MEM_SIZE) {
            fprintf(stderr, "Error: memoria llena en línea %d\n", nLinea);
            return 1;
        }

        memoria[pc++] = palabra;
    }

    fclose(programa);
}

void print_bin16(uint16_t value) {
    for (int i = 15; i >= 0; i--) {
        putchar((value & (1 << i)) ? '1' : '0');
        if (i % 4 == 0 && i != 0) putchar(' '); // separador cada 4 bits
    }
}

void read(uint8_t reg, uint16_t dir) {
    if (reg) {
        B = memoria[dir];
        z = (B == 0);
    } else {
        A = memoria[dir];
        z = (A == 0);
    }
}

/* ADD: Suma memoria[dir] al registro seleccionado */
void add(uint8_t reg, uint16_t dir) {
    if (reg) {
        uint32_t tmp = (uint32_t)B + memoria[dir];
        B = (uint16_t)tmp;
        z = (B == 0);
    } else {
        uint32_t tmp = (uint32_t)A + memoria[dir];
        A = (uint16_t)tmp;
        z = (A == 0);
    }
}

/* AND: AND bit a bit entre el registro y memoria[dir] */
void and(uint8_t reg, uint16_t dir) {
    if (reg) {
        B &= memoria[dir];
        z = (B == 0);
    } else {
        A &= memoria[dir];
        z = (A == 0);
    }
}

/* OR: OR bit a bit entre el registro y memoria[dir] */
void or(uint8_t reg, uint16_t dir) {
    if (reg) {
        B |= memoria[dir];
        z = (B == 0);
    } else {
        A |= memoria[dir];
        z = (A == 0);
    }
}

void instr_write(int registro_bit, uint16_t direccion) {
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

void instr_beq(uint16_t direccion) {
    if (z == 1) {
        pc = direccion; 
    }
}

void instr_sub(int registro_bit, uint16_t direccion) {
    uint16_t valor_memoria = memoria[direccion];
    if (registro_bit == 0) { 
        A = A - valor_memoria;
        z = (A == 0) ? 1 : 0;
    } else { 
        B = B - valor_memoria;

        z = (B == 0) ? 1 : 0;
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_del_archivo_del_programa>\n", argv[0]);
        return 1;
    }

    if (cargarPrograma(argv[1]) == 1) {
        return 1;
    }

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

    printf("Ejecución finalizada.\n");
    printf("Registro A = %u, Registro B = %u\n", A, B);

    /*
    printf("\n=== Primeras 10 posiciones de memoria ===\n");
    for (int i = 0; i < 10; i++) {
        printf("Dir 0x%03X : 0x%04X (", i, memoria[i]);
        print_bin16(memoria[i]);
        printf(")\n");
    }*/

    return 0;
}
