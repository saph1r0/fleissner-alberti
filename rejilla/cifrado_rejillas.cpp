/*
 * Cifrado por rejilla giratoria (Fleissner) - P1
 *
 * COMO USAR
 *   Compilar:   g++ -std=c++17 cifrado_rejillas.cpp -o cifrado_rejillas
 *   Ejecutar:   ./cifrado_rejillas
 *
 *   La rejilla es un cuadrado n x n con n par, escrito como vector<string>:
 *   'X' es un hueco y '.' es una casilla cerrada. Debe tener un solo hueco
 *   por cada grupo de 4 casillas que se corresponden al girar 90 grados.
 *
 *       vector<string> rejilla = {"X...",
 *                                 "...X",
 *                                 "..X.",
 *                                 ".X.."};
 *       string cifrado = cifrar("Ataque al amanecer", rejilla);
 *
 *   El texto se pasa a mayusculas sin tildes, espacios ni signos.
 *   Si el ultimo bloque queda incompleto se rellena con 'X'
 *   (otro relleno: cifrar(texto, rejilla, 'Q')).
 *   Si la rejilla no es valida se lanza std::invalid_argument.
 *   Si el texto no tiene letras ni numeros tambien se lanza invalid_argument.
 *   Para verificar: descifrar(cifrado, rejilla, longitud_original).
 *   Para usar cifrar() desde otro archivo, borra la funcion main.
 *
 *   Cambios tras las pruebas (P2):
 *   - normalizar reconoce todas las letras latinas con tilde (a-virgulilla, o-dieresis, c-cedilla...)
 *     y tambien texto en ANSI/Latin-1 (Bloc de notas, consola de Windows).
 *   - cifrar lanza un error si no queda nada que cifrar (antes devolvia "").
 *   - nueva funcion descifrar().
 */

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

using Posicion = pair<int, int>;

// Letra base de un caracter latino U+00C0..U+00FF; 0 si no es letra (signos de por y dividir).
static char letraBase(unsigned char cp) {
    static const char tabla[] = "AAAAAAACEEEEIIIIDNOOOOO.OUUUUYTS"   // U+00C0..U+00DF (mayusculas)
                                "AAAAAAACEEEEIIIIDNOOOOO.OUUUUYTY";  // U+00E0..U+00FF (minusculas)
    if (cp < 0xC0) return 0;
    char b = tabla[cp - 0xC0];
    return b == '.' ? 0 : b;
}

static string normalizar(const string& texto) {
    string salida;
    for (size_t i = 0; i < texto.size(); i++) {
        unsigned char c = texto[i];
        if (c < 128) {
            if (isalnum(c)) salida += (char)toupper(c);
        } else {
            bool sigueContinuacion = i + 1 < texto.size() && ((unsigned char)texto[i + 1] & 0xC0) == 0x80;
            char b = 0;
            if (c == 0xC3 && sigueContinuacion) {          // UTF-8: letra con tilde (2 bytes)
                unsigned char d = texto[++i];
                b = letraBase(d + 0x40);
            } else if (c >= 0xC0 && !sigueContinuacion) {  // ANSI/Latin-1: la letra ocupa 1 byte
                b = letraBase(c);
            }
            if (b) salida += b;
        }
    }
    return salida;
}

static vector<Posicion> leerHuecos(const vector<string>& rejilla) {
    int n = rejilla.size();
    vector<Posicion> huecos;
    for (int r = 0; r < n; r++) {
        if ((int)rejilla[r].size() != n)
            throw invalid_argument("La rejilla debe ser cuadrada.");
        for (int c = 0; c < n; c++) {
            char ch = rejilla[r][c];
            if (ch == 'X' || ch == 'x')
                huecos.push_back({r, c});
            else if (ch != '.')
                throw invalid_argument(string("Caracter no valido '") + ch + "' en (" +
                                       to_string(r) + ", " + to_string(c) + ").");
        }
    }
    return huecos;
}

int validarRejilla(const vector<string>& rejilla) {
    int n = rejilla.size();
    if (n == 0) throw invalid_argument("La rejilla esta vacia.");
    vector<Posicion> huecos = leerHuecos(rejilla);
    if (n % 2 != 0)
        throw invalid_argument("El lado de la rejilla debe ser par (recibido " +
                               to_string(n) + ").");

    vector<vector<int>> cobertura(n, vector<int>(n, 0));
    for (Posicion h : huecos) {
        int r = h.first, c = h.second;
        for (int giro = 0; giro < 4; giro++) {
            cobertura[r][c]++;
            int nr = c, nc = n - 1 - r;
            r = nr;
            c = nc;
        }
    }

    string repetidas, faltantes;
    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            string pos = "(" + to_string(r) + ", " + to_string(c) + ") ";
            if (cobertura[r][c] > 1) repetidas += pos;
            if (cobertura[r][c] == 0) faltantes += pos;
        }
    }
    if (!repetidas.empty() || !faltantes.empty()) {
        string mensaje = "Rejilla no valida;";
        if (!repetidas.empty())
            mensaje += " casillas descubiertas mas de una vez: " + repetidas;
        if (!faltantes.empty())
            mensaje += " casillas que nunca se descubren: " + faltantes;
        throw invalid_argument(mensaje);
    }
    return n;
}

string cifrar(const string& texto, const vector<string>& rejilla, char relleno = 'X') {
    int n = validarRejilla(rejilla);
    relleno = (char)toupper((unsigned char)relleno);
    if (!isalnum((unsigned char)relleno))
        throw invalid_argument("El relleno debe ser una letra o un digito.");

    vector<vector<Posicion>> giros;
    vector<Posicion> actual = leerHuecos(rejilla);
    for (int g = 0; g < 4; g++) {
        sort(actual.begin(), actual.end());
        giros.push_back(actual);
        for (Posicion& p : actual) {
            int r = p.first, c = p.second;
            p = {c, n - 1 - r};
        }
    }

    string claro = normalizar(texto);
    if (claro.empty())
        throw invalid_argument("No hay nada que cifrar: el texto no tiene letras ni numeros.");

    size_t tam = (size_t)n * n;
    claro.append((tam - claro.size() % tam) % tam, relleno);

    string salida;
    for (size_t inicio = 0; inicio < claro.size(); inicio += tam) {
        vector<string> cuadro(n, string(n, ' '));
        size_t k = inicio;
        for (const vector<Posicion>& giro : giros)
            for (const Posicion& p : giro)
                cuadro[p.first][p.second] = claro[k++];
        for (const string& fila : cuadro) salida += fila;
    }
    return salida;
}

// Operacion inversa: escribe el cifrado por filas en el cuadro y lo lee a traves
// de los huecos, en los mismos 4 giros. Con la longitud original quita el relleno.
string descifrar(const string& cifrado, const vector<string>& rejilla, int longitud = -1) {
    int n = validarRejilla(rejilla);

    vector<vector<Posicion>> giros;
    vector<Posicion> actual = leerHuecos(rejilla);
    for (int g = 0; g < 4; g++) {
        sort(actual.begin(), actual.end());
        giros.push_back(actual);
        for (Posicion& p : actual) {
            int r = p.first, c = p.second;
            p = {c, n - 1 - r};
        }
    }

    string limpio;  // se ignoran espacios y saltos de linea que se cuelen al copiar
    for (char ch : cifrado)
        if (!isspace((unsigned char)ch)) limpio += (char)toupper((unsigned char)ch);
    size_t tam = (size_t)n * n;
    if (limpio.empty() || limpio.size() % tam != 0)
        throw invalid_argument("El cifrado tiene " + to_string(limpio.size()) +
                               " caracteres; deberia ser un multiplo de " + to_string(tam) + ".");

    string claro;
    for (size_t inicio = 0; inicio < limpio.size(); inicio += tam)
        for (const vector<Posicion>& giro : giros)
            for (const Posicion& p : giro)
                claro += limpio[inicio + p.first * n + p.second];

    if (longitud >= 0) {
        if ((size_t)longitud > claro.size())
            throw invalid_argument("La longitud original es mayor que el texto descifrado.");
        claro.resize(longitud);
    }
    return claro;
}

int main() {
    vector<string> rejilla = {"X...",
                              "...X",
                              "..X.",
                              ".X.."};
    try {
        string mensaje = "Ataque al amanecer";
        cout << "Texto claro  : " << mensaje << endl;
        cout << "Texto cifrado: " << cifrar(mensaje, rejilla) << endl;
    } catch (const invalid_argument& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
