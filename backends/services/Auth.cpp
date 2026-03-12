#include "Auth.h"
#include <sodium.h>
#include <stdexcept>

Auth::Auth()
{
    // This constructor will be called several times.
    // You can add static guard, but you don't have to...
    if (sodium_init() < 0) {
        throw runtime_error("libsodium init failed");
    }
}

/*
 * verifyHash
 *
 * Description:
 *   Verifies whether a given plain text input matches
 *   a stored Argon2id hash string.
 *
 * Parameters:
 *   storedHash - Argon2id hash retrieved from the database.
 *
 *   input      - Plain text value entered by the user
 *                (password or PIN).
 *
 * Returns:
 *   true  - if the input matches the stored hash
 *   false - if the hash is empty or verification fails
 */
bool Auth::verifyHash(const char* storedHash, const char* input, size_t inputLen) 
{
    // check if the stored hashed password is not empty.
    if (!storedHash || storedHash[0] == '\0') {
        return false;
    }

    // check if the typed password is correct
    return (crypto_pwhash_str_verify(
            storedHash,
            input,
            inputLen)
        ) == 0;
}

/*
 * verifyPassword
 *
 * Description:
 *   Retrieves the stored password hash for a specific user
 *   and verifies it against the entered password.
 *
 * Parameters:
 *   userId        - Used to locate the user's password hash.
 *   inputPassword - Plain text password entered by the user.
 *
 * Returns:
 *   true  - if the password is correct
 *   false - if the password is incorrect or the stored hashed password is empty.
 */
Result<bool> Auth::verifyPassword(const char* username, const char* inputPassword, size_t inputPasswordLen, const char* storedHash) 
{
    Result<bool> result;
    result.value = false;
    if (!storedHash || storedHash[0] == '\0')
    {
        result.error = AppError(ErrorType::Database, "Login failed.");
        return result;
    }
    if (!verifyHash(storedHash, inputPassword, inputPasswordLen))
    {
        result.error = AppError(ErrorType::Database, "Login failed.");
        return result;
    }
    result.value = true;
    return result;
}

/*
 * verifyPin
 *
 * Description:
 *   Retrieves the stored pin hash for a specific user
 *   and verifies it against the entered pin.
 *
 * Parameters:
 *   userId   - Used to locate the user's pin hash.
 *   inputPin - Plain text pin entered by the user.
 *
 * Returns:
 *   true  - if the pin is correct
 *   false - if the pin is incorrect or the stored hashed pin is empty.
 */
bool Auth::verifyPin(const int userId, const char* inputPin, size_t inputPinLen) 
{
    // string hash = repo.get_pin_hash(userId);
    const char* hash = "1234";
    return verifyHash(hash, inputPin, inputPinLen);
}


/*
 * createHashPassword
 *
 * Description:
 *   Generates an Argon2id hash from the user's plain text password.
 *
 * Parameters:
 *   inputPassword - Plain text password to be hashed.
 */
string Auth::createHashPassword(const char* inputPassword, size_t passwordLen) 
{
    // This is a buffer that stores hashed result as a string.
    char out[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(
            out,
            inputPassword,
            passwordLen,
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE
        ) != 0) {
        throw runtime_error("crypto_pwhash_str failed.");
    }

    return string(out);

}

/*
 * createHashPin
 *
 * Description:
 *   Generates an Argon2id hash from the user's plain text pin.
 *
 * Parameters:
 *   inputPin  - Plain text pin to be hashed.
 */
string Auth::createHashPin(const char* inputPin, size_t pinLen) 
{
    // This is a buffer that stores hashed result as a string.
    char out[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(
            out,
            inputPin,
            pinLen,
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE
        ) != 0) {
        throw runtime_error("crypto_pwhash_str failed.");
    }

    return string(out);
}