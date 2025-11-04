#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MEM_SIZE 4096

uint16_t memoria[MEM_SIZE];

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


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_del_archivo_del_programa>\n", argv[0]);
        return 1;
    }

    if (cargarPrograma(argv[1]) == 1) {
        return 1;
    }

    printf("\n=== Primeras 10 posiciones de memoria ===\n");
    for (int i = 0; i < 10; i++) {
        printf("Dir 0x%03X : 0x%04X (", i, memoria[i]);
        print_bin16(memoria[i]);
        printf(")\n");
    }

    return 0;
}
