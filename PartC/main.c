/* Main source file for Assignment 2, Part A. This program demonstrates a simple encryption flow 
 * using a custom packetizer and libsodium for cryptographic operations. It also includes a 
 * stack-based buffer overflow vulnerability for educational purposes.
 * **************************************************************************************************/

#include "sodium.h"
#include "system_XMC4500.h"

#include "VirtualSerial.h"
#include "packetizer.h"
#include "crypto.h"

// Concatenates the Unique Chip ID (UCI) to create a 32-byte key
// The UCI is repeated twice to fill the key buffer
void ConcatenateUCI(uint8_t* key, const uint8_t* UCI, int lenOfUCI)
{ 
  // lenOfUCI is 16 in this implementation
  for (int i = 0; i < lenOfUCI; i++)
  {
    key[i] = UCI[i];
    key[i + lenOfUCI] = UCI[i];
  }
}

void freeAllDynamicMemoryAllocations(void* memRegion)
{
      // Free dynamically allocated buffer to prevent memory leaks
      sodium_free(memRegion);
      // Reset pointer to NULL after freeing memory
      memRegion = NULL;
}

int main(void) 
{
  // Retrieve the Unique Chip ID (UCI) from the system_XMC4500.h file
  extern uint8_t g_chipid[16];
  uint8_t key[32] = {0};
  ConcatenateUCI(key, g_chipid, 16);

  // Initialize plaintext and ciphertext structures
  pt_s plaintext = { 0 }; // Input data to be encrypted
  ct_s ciphertext = { 0 }; // Encrypted output data

  // Initialize USB communication and the random number generator
  USB_Init();
  randombytes_set_implementation(&randombytes_stir);

  // Initialize the libsodium library. 
  // Exit if initialization fails
  sodium_init();
  /*
  if (sodium_init() == -1) 
  {
    return -1; // Sodium initialization failed
  }
  */

  while(1) 
  {
    // Wait for incoming data and parse it into a valid packet
    // If a valid packet is received, continue encryption process
    if(packetizerReceive(&plaintext) == 0) 
    {
      // If the sender did not provide a nonce, generate one
      if(plaintext.nonce == NULL) 
      {
        plaintext.nonce = sodium_malloc(crypto_secretbox_NONCEBYTES);
        if (plaintext.nonce == NULL)
        {
          return -2; // Memory allocation for nonce failed
        }
        randombytes_buf(plaintext.nonce, crypto_secretbox_NONCEBYTES); // Generate a random nonce
      }

      // Allocate memory for the ciphertext buffer to store the encrypted data.
      // The size must be equal to plaintext length plus the MAC bytes added by the encryption.
      ciphertext.text = sodium_malloc(plaintext.textLen + crypto_secretbox_MACBYTES);
      if (ciphertext.text == NULL) 
      {
        return -3; // Memory allocation for ciphertext failed.
      }

      // Encrypt the plaintext using the generated or provided nonce. 
      // crypto_secretbox_easy returns 0 in case of success.
      if(crypto_secretbox_easy(&ciphertext.text, plaintext.text, plaintext.textLen, plaintext.nonce, key) == 0) 
      {
        ciphertext.textLen = plaintext.textLen + crypto_secretbox_MACBYTES;
        // Send the encrypted data back as a packet.
        packetizerSend(&ciphertext);
      }

      // Free all dynamically allocated buffers to prevent memory leaks.
      freeAllDynamicMemoryAllocations(plaintext.nonce);
      freeAllDynamicMemoryAllocations(plaintext.text);
      freeAllDynamicMemoryAllocations(ciphertext.text);
    }
  }
}
