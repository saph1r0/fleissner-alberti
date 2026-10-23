/*
 * pruebas.cpp - Pruebas estrictas del codigo de P1 (P2)
 *
 * Incluye cifrado_rejillas.cpp y renombra su main para no chocar con este.
 *
 *   g++ -std=c++17 pruebas.cpp -o pruebas      (NO compilar cifrado_rejillas.cpp aparte)
 *   ./pruebas
 */
#ifdef _WIN32  // antes del codigo de P1 (por su "using namespace std")
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#define main main_original_p1
#include "cifrado_rejillas.cpp"
#undef main

#include <random>

#include "casos_prueba.hpp"


int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    vector<prueba::Resultado> res = prueba::ejecutarTodos();
    int pasan = 0;
    string grupo;
    for (const prueba::Resultado& r : res) {
        if (r.caso.grupo != grupo) {
            grupo = r.caso.grupo;
            cout << "\n== " << grupo << " ==\n";
        }
        pasan += r.ok;
        cout << (r.ok ? "  PASA   " : "  FALLA  ") << r.caso.nombre << "\n";
        if (!r.ok) {
            cout << "           esperado: " << r.esperado << "\n"
                 << "           obtenido: " << (r.obtenido.empty() ? "(vacio)" : r.obtenido) << "\n"
                 << "           " << r.nota << "\n";
        }
    }
    cout << "\nRESULTADO: " << pasan << " de " << res.size() << " casos pasan, "
         << res.size() - pasan << " fallan.\n";

    // Comparacion masiva contra el cifrador de referencia
    std::mt19937 rng(2026);
    const vector<string> espanol = {"a", "E", "z", "7", " ", ",", "\xC2\xBF", "!", "\xC3\xA1", "\xC3\x89",
                                    "\xC3\xAD", "\xC3\xB3", "\xC3\x9A", "\xC3\xB1", "\xC3\x91", "\xC3\xBC", "\n"};
    const vector<string> otras = {"\xC3\xA0", "\xC3\xA7", "\xC3\xB6", "\xC3\xA3", "\xC3\xAA", "\xC3\x82"};
    auto normRef = [](const string& t) {  // normalizacion esperada para estos alfabetos
        string s;
        for (size_t i = 0; i < t.size(); i++) {
            unsigned char c = t[i];
            if (c < 128) { if (isalnum(c)) s += (char)toupper(c); continue; }
            if (c == 0xC2) { i++; continue; }  // ¿ ¡
            unsigned char d = t[++i];
            switch (d) {
                case 0xA1: case 0x81: case 0xA0: case 0xA3: case 0x82: s += 'A'; break;
                case 0x89: case 0xA9: case 0xAA: s += 'E'; break;
                case 0xAD: s += 'I'; break;
                case 0xB3: case 0xB6: s += 'O'; break;
                case 0x9A: case 0xBC: s += 'U'; break;
                case 0xB1: case 0x91: s += 'N'; break;
                case 0xA7: s += 'C'; break;
            }
        }
        return s;
    };
    int igualesEs = 0, totalEs = 0, igualesOtras = 0, totalOtras = 0, idaVuelta = 0;
    for (int i = 0; i < 4000; i++) {
        bool conOtras = i >= 2000;
        string t;
        int largo = 1 + rng() % 120;
        for (int k = 0; k < largo; k++) {
            if (conOtras && rng() % 8 == 0) t += otras[rng() % otras.size()];
            else t += espanol[rng() % espanol.size()];
        }
        string norm = normRef(t);
        if (norm.empty()) continue;
        int n = 2 * (1 + rng() % 4);
        vector<string> rej(n, string(n, '.'));
        for (int f = 0; f < n / 2; f++)
            for (int c = 0; c < n / 2; c++) {
                int r = f, cc = c;
                for (int g = rng() % 4; g > 0; g--) { int nr = cc; cc = n - 1 - r; r = nr; }
                rej[r][cc] = 'X';
            }
        string cif = cifrar(t, rej);
        bool igual = cif == prueba::cifrarReferencia(norm, rej);
        idaVuelta += (descifrar(cif, rej, (int)norm.size()) == norm);
        if (conOtras) { totalOtras++; igualesOtras += igual; }
        else { totalEs++; igualesEs += igual; }
    }
    cout << "\nComparacion con el cifrador de referencia (textos al azar):\n"
         << "  Solo letras del espanol : " << igualesEs << " de " << totalEs << " iguales\n"
         << "  Con otras letras (a-grave, c-cedilla, o-dieresis, a-virgulilla...): " << igualesOtras
         << " de " << totalOtras << " iguales\n"
         << "  Ida y vuelta (cifrar -> descifrar): " << idaVuelta << " de " << totalEs + totalOtras << " correctos\n";
    bool todo = pasan == (int)res.size() && igualesEs == totalEs && igualesOtras == totalOtras &&
                idaVuelta == totalEs + totalOtras;
    cout << (todo ? "\nTODO CORRECTO\n" : "\nHAY FALLAS\n");
    return todo ? 0 : 1;
}
