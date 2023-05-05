/* *********************************************************************
 * This Original Work is copyright of 51 Degrees Mobile Experts Limited.
 * Copyright 2022 51 Degrees Mobile Experts Limited, Davidson House,
 * Forbury Square, Reading, Berkshire, United Kingdom RG1 3EU.
 *
 * This Original Work is licensed under the European Union Public Licence
 * (EUPL) v.1.2 and is subject to its terms as set out below.
 *
 * If a copy of the EUPL was not distributed with this file, You can obtain
 * one at https://opensource.org/licenses/EUPL-1.2.
 *
 * The 'Compatible Licences' set out in the Appendix to the EUPL (as may be
 * amended by the European Commission) shall be deemed incompatible for
 * the purposes of the Work and the provisions of the compatibility
 * clause in Article 5 of the EUPL shall not apply.
 *
 * If using the Work as, or as part of, a network application, by
 * including the attribution notice(s) required under Article 5 of the EUPL
 * in the end user terms of the application under an appropriate heading,
 * such notice(s) shall fulfill the requirements of that article.
 * ********************************************************************* */

#include "yamlfile.h"
#include "fiftyone.h"

typedef struct file_state_t {
	FILE* handle;
	char* buffer;
	size_t current;
	size_t readLength;
	size_t length;
	size_t dashCount;
	size_t dotCount;
} FileState;

static StatusCode readNextBlock(FileState* fileState) {
	StatusCode status = SUCCESS;
	size_t bytesRead = fread(fileState->buffer,
		sizeof(char),
		fileState->length,
		fileState->handle);
	if (bytesRead == 0 && !feof(fileState->handle)) {
		status = INSUFFICIENT_MEMORY;
	}
	else if (bytesRead < fileState->readLength
		&& ferror(fileState->handle)) {
		status = FILE_READ_ERROR;
	}
	else if (bytesRead < fileState->readLength
		&& feof(fileState->handle)) {
		fileState->readLength = bytesRead;
		status = FILE_END_OF_FILE;
	}
	else {
		fileState->readLength = bytesRead;
	}
	fileState->current = 0;
	return status;
}

// Skip all white spaces
static StatusCode skipWhiteSpace(FileState* fileState) {
	StatusCode status = SUCCESS;
	do {
		while (fileState->current < fileState->readLength
			&& *(fileState->buffer + fileState->current) == ' ') fileState->current++;
		if (fileState->current == fileState->readLength) {
			status = readNextBlock(fileState);
			if (status != SUCCESS){
				return status;
			}
		}
		else {
			break;
		}
	} while (true);
	return status;
}

static StatusCode skipLine(FileState *fileState) {
	StatusCode status = SUCCESS;
	bool eol = false;
	do {
		// Read until end of line or end of the buffer is reached
		while (fileState->current < fileState->readLength && eol == false) {
			if (*(fileState->buffer + fileState->current) == '\n' || 
				*(fileState->buffer + fileState->current) == '\r') {
				eol = true;
				break;
			}
			fileState->current++;
		}

		// Skip eol
		while (fileState->current < fileState->readLength
			&& eol == true
			&& (*(fileState->buffer + fileState->current) == '\n' 
				|| *(fileState->buffer + fileState->current) == '\r')) {
			fileState->current++;
		}

		if (fileState->current == fileState->readLength) {
			status = readNextBlock(fileState);
			if (status != SUCCESS) {
				return status;
			}
		}
		else {
			break;
		}
	} while (true);
	return status;
}

// Read a key and make sure the cursor is updated to the beginning
// of the value space if it is possible.
static StatusCode readKey(FileState *fileState, KeyValuePair* pair) {
	StatusCode status = SUCCESS;
	size_t filledKeyLength = 0;
	size_t count = 0;
	bool separatorFound = false;
	do {
		count = 0;
		while (fileState->current < fileState->readLength) {
			if (*(fileState->buffer + fileState->current) == ':') {
				separatorFound = true;
				break;
			}
			
			// Check document start ---
			if (*(fileState->buffer + fileState->current) == '-') {
				if (++fileState->dashCount == 3) {
					fileState->dashCount = 0;
					return FILE_END_OF_DOCUMENT;
				}
			}
			else {
				fileState->dashCount = 0;
			}

			// Check end of documents ...
			if (*(fileState->buffer + fileState->current) == '.') {
				if (++fileState->dotCount == 3) {
					fileState->dotCount = 0;
					return FILE_END_OF_DOCUMENTS;
				}
			}
			else {
				fileState->dotCount = 0;
			}

			if (filledKeyLength + count > pair->keyLength) {
				return INSUFFICIENT_MEMORY;
			}
			fileState->current++;
			count++;
		}

		// Move whichever read to the key space
		strncpy(pair->key + filledKeyLength, fileState->buffer + fileState->current - count, count);
		filledKeyLength += count;

		// End of buffer has been reached, read next block
		// and move cursor to the next byte in the block
		if (fileState->current == fileState->readLength) {
			status = readNextBlock(fileState);
			if ((status != SUCCESS &&
				status != FILE_END_OF_FILE) ||
				(status == FILE_END_OF_FILE &&
				fileState->readLength == 0)) {
				break;
			}
		}
		else if (separatorFound) {
			fileState->current++;
			break;
		}
	} while (true);

	// Set the null terminator
	if (filledKeyLength < pair->keyLength) {
		pair->key[filledKeyLength] = '\0';
	}
	else {
		pair->key[pair->keyLength] = '\0';
	}
	return SUCCESS;
}

// Read a value and make sure the cursor starts at the beginning of
// the next line.
StatusCode readValue(FileState *fileState, KeyValuePair* pair) {
	StatusCode status = SUCCESS;
	size_t count = 0;
	size_t filledValueLength = 0;
	bool eol = false;
	do {
		count = 0;
		// Read until end of line or end of the buffer is reached
		while (fileState->current < fileState->readLength && eol == false) {
			if (*(fileState->buffer + fileState->current) == '\n' 
				|| *(fileState->buffer + fileState->current) == '\r') {
				eol = true;
				break;
			}
			if (filledValueLength + count > pair->valueLength) {
				return INSUFFICIENT_MEMORY;
			}
			fileState->current++;
			count++;
		}

		// Move whichever read to the key space
		strncpy(
			pair->value + filledValueLength,
			fileState->buffer + fileState->current - count,
			count);
		filledValueLength += count;

		// Skip eol
		while (fileState->current < fileState->readLength
			&& eol == true 
			&& (*(fileState->buffer + fileState->current) == '\n' 
				|| *(fileState->buffer + fileState->current) == '\r')) {
			fileState->current++;
			count++;
		}

		if (fileState->current == fileState->readLength) {
			status = readNextBlock(fileState);
			if ((status != SUCCESS &&
				status != FILE_END_OF_FILE) ||
				(status == FILE_END_OF_FILE &&
					fileState->readLength == 0)) {
				break;
			}
		}
		else if (eol) {
			break;
		}
	} while (true);

	// Set the null terminator
	if (filledValueLength < pair->valueLength) {
		pair->value[filledValueLength] = '\0';
	}
	else {
		pair->value[pair->valueLength] = '\0';
	}
	return status;
}

StatusCode readNextKeyValuePair(FileState *fileState, KeyValuePair *pair) {
	StatusCode status;
	if ((status = readKey(fileState, pair)) != SUCCESS) {
		return status;
	}

	if ((status = skipWhiteSpace(fileState)) != SUCCESS) {
		return status;
	}

	if ((status = readValue(fileState, pair)) != SUCCESS) {
		return status;
	}

	return status;
}

StatusCode readNextDocument(
	FileState *fileState,
	KeyValuePair* keyValuePairs,
	uint16_t collectionSize,
	void *state,
	void(*callback)(KeyValuePair*, uint16_t, void*)) {
	StatusCode status = skipLine(fileState);
	if (status != SUCCESS) {
		return status;
	}

	uint16_t count = 0;
	while (
		(status = readNextKeyValuePair(
			fileState,
			&keyValuePairs[count])) == SUCCESS && count < collectionSize) {
		count++;
	}

	// Only call back if succeeded
	if (status == SUCCESS ||
		status == FILE_END_OF_DOCUMENT ||
		status == FILE_END_OF_DOCUMENTS)
	{
		callback(keyValuePairs, count, state);
	}
	return status;
}

StatusCode fiftyoneDegreesYamlFileIterateWithLimit(
	const char* fileName,
	char* buffer,
	size_t length,
	KeyValuePair* keyValuePairs,
	uint16_t collectionSize,
	int limit,
	void* state,
	void(*callback)(KeyValuePair*, uint16_t, void*)) {
	FILE *handle;
	int count = 0;

	StatusCode status = FileOpen(fileName, &handle);
	FileState fileState = { handle, buffer, 0, 0, length, 0, 0 };
	if (status == SUCCESS) {
		// Read the first block
		status = readNextBlock(&fileState);
		if (status == SUCCESS ||
			status == FILE_END_OF_FILE) {
			while ((count < limit || limit < 0)
				&& (status == SUCCESS || status == FILE_END_OF_DOCUMENT)) {
				status = readNextDocument(
					&fileState,
					keyValuePairs,
					collectionSize,
					state,
					callback);
				count++;
			}
		}
	}
	fclose(handle);
	return status;
}

StatusCode fiftyoneDegreesYamlFileIterate(
	const char* fileName,
	char* buffer,
	size_t length,
	KeyValuePair* keyValuePairs,
	uint16_t collectionSize,
	void* state,
	void(*callback)(KeyValuePair*, uint16_t, void*)) {
	return fiftyoneDegreesYamlFileIterateWithLimit(
		fileName,
		buffer,
		length,
		keyValuePairs,
		collectionSize,
		-1,
		state,
		callback
	);
}