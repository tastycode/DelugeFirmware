#include "syxpack.h"

/*
 *
 * Utility used by loadfw to unpack response values from validation failure
 * really only used to call unpack_7bit_to_8bit on every 5 bytes so that
 * python can retransmit those packets
 *
 * build
 * gcc -g -w contrib/syxpack/syxpack.c src/deluge/util/pack.c -Isrc/deluge -Icontrib/load  -I/opt/homebrew/Cellar/argp-standalone/1.3/include -L/opt/homebrew/Cellar/argp-standalone/1.3/lib -largp -o ./toolchain/darwin-arm64/syxpack
 *
 */

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'u': arguments->unpack = true; break;
        case 'b': arguments->binary = true; break;
        case 'd': arguments->output_decimal = true; break;
        case 'i': arguments->input_decimal = true; break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 1) argp_usage(state);
            arguments->args[state->arg_num] = arg;
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 1) argp_usage(state);
            break;
        default: return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp_option options[] = {
    {"unpack", 'u', 0, 0, "Unpack data"},
    {"binary", 'b', 0, 0, "Handle binary data"},
    {"output_decimal", 'd', 0, 0, "Print as decimal output"},
    {"input_decimal", 'i', 0, 0, "Interpret input as decimal"},
    {0}
};

static char args_doc[] = "DATA";
static char doc[] = "SyxPack - A tool to pack and unpack SysEx data";

static struct argp argp = {options, parse_opt, args_doc, doc};


uint8_t hexCharToVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return 0;
}

uint8_t* hexStringToUint8(const char* hexString, size_t* outSize) {
    size_t hexLen = strlen(hexString);
    size_t bytesLen = 0;
    for (size_t i = 0; i < hexLen; ++i) {
        if (!isspace(hexString[i])) {
            bytesLen++;
        }
    }
    bytesLen = bytesLen / 2; // Two hex chars per byte

    uint8_t* byteArray = malloc(bytesLen);
    if (!byteArray) {
        perror("Failed to allocate memory");
        return NULL;
    }

    size_t byteIndex = 0;
    for (size_t i = 0; i < hexLen; ++i) {
        if (isspace(hexString[i])) continue; // Skip whitespace

        if (i + 1 < hexLen) {
            byteArray[byteIndex] = hexCharToVal(hexString[i]) * 16 + hexCharToVal(hexString[i + 1]);
            i++; // Skip next char as it's part of the same byte
            byteIndex++;
        }
    }

    if (outSize != NULL) {
        *outSize = bytesLen;
    }
    return byteArray;
}
char* uint8ToHexString(const uint8_t* inputBytes, size_t inputSize) {
    // Each byte will be represented by 2 hex characters, +1 for null terminator
    char* hexString = malloc(inputSize * 2 + 1);
    if (!hexString) {
        perror("Failed to allocate memory for hex string");
        return NULL;
    }

    for (size_t i = 0; i < inputSize; ++i) {
        sprintf(&hexString[i * 2], "%02x", inputBytes[i]);
    }

    hexString[inputSize * 2] = '\0'; // Null-terminate the string
    return hexString;
}

int main(int argc, char **argv) {
	char* inputText;
	uint8_t* inputBytes;
	int bytesRead;
	size_t inputSize;
	uint8_t* outputBytes;
	size_t outputSize;
	char* outputText;
    struct arguments arguments;

    // Default values
    arguments.unpack = false;
    arguments.binary = false;
    arguments.output_decimal = false;
    arguments.input_decimal = false;
	arguments.args[0] = NULL;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

	if (arguments.binary) {
		printf("Not implemented. Use xxd -p to convert binary to hex. xxd -r -p to convert from hex");
		exit(1);
	}

	if (arguments.args[0] == NULL) {
        // No data argument provided, read from stdin
        printf("Enter hex data: ");
        bytesRead = getline(&inputText, &inputSize, stdin);
        if (bytesRead == -1) {
            perror("Failed to read input");
            return 1;
        }

        if (inputText[bytesRead - 1] == '\n') {
            inputText[bytesRead - 1] = '\0'; // Remove newline character
        }
    } else {
        // Use the provided data argument
        inputText = strdup(arguments.args[0]);
    }


    // Convert the hex string to uint8_t array
    inputBytes = hexStringToUint8(inputText, &inputSize);
    if (!inputBytes) {
        fprintf(stderr, "Failed to convert hex to bytes\n");
        free(inputText);
        return 1;
    }
	outputSize = arguments.unpack ? unpacked_size_8(inputSize) : packed_size_7(inputSize);
    outputBytes = malloc(outputSize);
    if (!outputBytes) {
        perror("Failed to allocate memory for outputBytes");
        free(inputText);
        free(inputBytes);
        return 1;
    }


	if (arguments.unpack) {
		unpack_7bit_to_8bit(outputBytes, outputSize, inputBytes, inputSize);
	} else {
		pack_8bit_to_7bit(outputBytes, outputSize, inputBytes, inputSize);
	}
	if (arguments.output_decimal) {
		int32_t* intValuePointer = (int32_t*)outputBytes;
		int32_t intValue = *intValuePointer;
		printf("%d\n", intValue);
	} else {
		outputText = uint8ToHexString(outputBytes, outputSize);
		printf("%s\n", outputText);
	}

	free(inputText);
	free(inputBytes);
	free(outputBytes);
	free(outputText);
    return 0;
}
