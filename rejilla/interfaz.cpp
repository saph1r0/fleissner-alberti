/*
 * interfaz.cpp - Interfaz de consola para el cifrado por rejilla (P2)
 *
 * Usa las funciones de P1 (cifrado_rejillas.cpp): se incluye aqui y su main se
 * renombra para que no choque con el de la interfaz.
 *
 *   Compilar:  g++ -std=c++17 interfaz.cpp -o rejilla      (NO compilar cifrado_rejillas.cpp aparte)
 *   Ejecutar:  ./rejilla        (Windows: rejilla.exe)
 *
 * Menu:
 *   1) Cifrar texto                     4) Cambiar rejilla
 *   2) Ver paso a paso el ultimo cifrado 5) Opciones (relleno, colores)
 *   3) Descifrar / verificar            6) Casos de prueba
 *   0) Salir
 */
#ifdef _WIN32  // antes del codigo de P1 (por su "using namespace std")
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#define main main_original_p1
#include "cifrado_rejillas.cpp"
#undef main

#include <fstream>
#include <random>

#include "casos_prueba.hpp"

// ---------------------------------------------------------------------------
// Consola
// ---------------------------------------------------------------------------
static bool colores = true;
static bool finEntrada = false;

static string col(const char* c) { return colores ? string("\033[") + c + "m" : ""; }
static string RST() { return col("0"); }
static string NEG() { return col("1"); }
static string AZUL() { return col("1;94"); }
static string VERDE() { return col("1;92"); }
static string ROJO() { return col("1;91"); }
static string AMAR() { return col("1;93"); }
static string GRIS() { return col("90"); }

static void prepararConsola() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo = 0;
    if (GetConsoleMode(out, &modo) && !SetConsoleMode(out, modo | 0x0004)) colores = false;
#endif
}

// Lee una linea en UTF-8 (en Windows con ReadConsoleW para no perder tildes).
static string leerLinea() {
#ifdef _WIN32
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    DWORD modo;
    if (GetConsoleMode(in, &modo)) {
        std::wstring w;
        wchar_t buf[1024];
        DWORD leidos = 0;
        while (true) {
            if (!ReadConsoleW(in, buf, 1024, &leidos, nullptr) || leidos == 0) { finEntrada = true; break; }
            w.append(buf, leidos);
            if (!w.empty() && w.back() == L'\n') break;
        }
        while (!w.empty() && (w.back() == L'\n' || w.back() == L'\r')) w.pop_back();
        if (w.empty()) return "";
        int tam = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        string s(tam, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], tam, nullptr, nullptr);
        return s;
    }
#endif
    string s;
    if (!getline(cin, s)) finEntrada = true;
    if (!s.empty() && s.back() == '\r') s.pop_back();
    return s;
}

static string preguntar(const string& m) {
    cout << AMAR() << m << RST();
    cout.flush();
    return leerLinea();
}

static void pausa() { if (!finEntrada) preguntar("\n[Enter para volver al menu] "); }

static void limpiar() { cout << (colores ? "\033[2J\033[H" : "\n\n"); }

static string leerTextoLargo() {
    cout << GRIS() << "(Escribe o pega el texto; puede tener varias lineas. Linea vacia para terminar.)" << RST() << "\n";
    string texto;
    bool primera = true;
    while (!finEntrada) {
        string l = leerLinea();
        if (l.empty()) break;
        if (!primera) texto += "\n";
        texto += l;
        primera = false;
    }
    return texto;
}

// Letras que normalizar() no reconoce y se eliminarian (fuera de A-Z y de À..ÿ,
// por ejemplo č ő ł ş).
static string letrasQueSePierden(const string& t) {
    string perdidas;
    for (size_t i = 0; i < t.size();) {
        unsigned char c = t[i];
        if (c < 0xC4 || c > 0xC9 || i + 1 >= t.size() || ((unsigned char)t[i + 1] & 0xC0) != 0x80) { i++; continue; }
        string letra = t.substr(i, 2);  // U+0100..U+024F: letras latinas extendidas
        if (perdidas.find(letra) == string::npos) perdidas += letra + " ";
        i += 2;
    }
    return perdidas;
}

// ---------------------------------------------------------------------------
// Estado y dibujo
// ---------------------------------------------------------------------------
struct Estado {
    vector<string> rejilla = {"X...", "...X", "..X.", ".X.."};
    char relleno = 'X';
    string ultimoTexto;      // texto original del ultimo cifrado
    string ultimoCifrado;
    int ultimaLongitud = -1; // letras del texto normalizado
};

static string rejillaTexto(const vector<string>& r) {
    string s;
    for (size_t i = 0; i < r.size(); i++) s += (i ? "/" : "") + r[i];
    return s;
}

// tipo: 0 vacia, 1 hueco del giro actual, 2 ya escrita, 3 relleno, 4 hueco (dibujo de rejilla)
static void dibujar(int n, const vector<string>& letras, const vector<vector<int>>& tipo) {
    for (int f = 0; f < n; f++) {
        cout << "    ";
        for (int c = 0; c < n; c++) {
            int t = tipo[f][c];
            char ch = letras.empty() ? ' ' : letras[f][c];
            if (t == 0 || t == 4) ch = ' ';
            if (colores) {
                const char* fondo = (t == 1 || t == 4) ? "44;97;1" : t == 2 ? "100;97" : t == 3 ? "43;30" : "47;30";
                cout << col(fondo) << " " << ch << " " << RST() << " ";
            } else {
                if (t == 4) cout << "[ ] ";
                else if (t == 1) cout << "[" << ch << "] ";
                else if (t == 0) cout << " .  ";
                else cout << " " << ch << "  ";
            }
        }
        cout << "\n";
    }
}

static void dibujarRejilla(const vector<string>& r) {
    int n = r.size();
    vector<vector<int>> tipo(n, vector<int>(n, 0));
    for (int f = 0; f < n; f++)
        for (int c = 0; c < n && c < (int)r[f].size(); c++)
            if (r[f][c] == 'X' || r[f][c] == 'x') tipo[f][c] = 4;
    dibujar(n, {}, tipo);
}

// ---------------------------------------------------------------------------
// 1) Cifrar
// ---------------------------------------------------------------------------
static void accionCifrar(Estado& e) {
    cout << NEG() << "1. CIFRAR\n" << RST();
    string texto = leerTextoLargo();
    if (texto.empty()) { cout << "  (Cancelado)\n"; return; }

    string perdidas = letrasQueSePierden(texto);
    if (!perdidas.empty())
        cout << ROJO() << "\n  ATENCION: estas letras no se reconocen y se van a eliminar: " << perdidas << RST()
             << "\n  Conviene escribirlas sin tilde y volver a cifrar.\n";
    try {
        string cif = cifrar(texto, e.rejilla, e.relleno);  // funcion de P1
        string claro = normalizar(texto);
        int n = e.rejilla.size();
        size_t tam = (size_t)n * n;
        e.ultimoTexto = texto;
        e.ultimoCifrado = cif;
        e.ultimaLongitud = (int)claro.size();
        cout << "\n  Texto que se cifra (" << claro.size() << " letras):\n    " << claro << "\n";
        cout << "\n  " << VERDE() << "TEXTO CIFRADO (" << cif.size() << " caracteres):" << RST() << "\n\n    "
             << NEG() << cif << RST() << "\n\n";
        cout << "  " << cif.size() / tam << " bloque(s) de " << n << "x" << n << ", " << cif.size() - claro.size()
             << " caracteres de relleno.\n";
        cout << "  Para el grupo que descifra: rejilla " << rejillaTexto(e.rejilla) << ", giro horario, relleno "
             << (char)toupper((unsigned char)e.relleno) << ", longitud original " << claro.size() << ".\n";
        std::ofstream f("ultimo_cifrado.txt", std::ios::binary);
        if (f) {
            f << cif << "\n\nRejilla: " << rejillaTexto(e.rejilla) << "\nGiro: horario | Relleno: "
              << (char)toupper((unsigned char)e.relleno) << " | Longitud original: " << claro.size() << "\n";
            cout << GRIS() << "  (Guardado tambien en ultimo_cifrado.txt)" << RST() << "\n";
        }
    } catch (const invalid_argument& ex) {
        cout << ROJO() << "\n  No se pudo cifrar: " << ex.what() << RST() << "\n";
    }
}

// ---------------------------------------------------------------------------
// 2) Paso a paso del ultimo cifrado
// ---------------------------------------------------------------------------
static void accionPasoAPaso(Estado& e) {
    cout << NEG() << "2. PASO A PASO\n" << RST();
    string texto = e.ultimoTexto;
    if (texto.empty()) {
        texto = "Cifrado en clase";
        cout << "  Aun no se cifro nada; uso el ejemplo \"" << texto << "\".\n";
    }
    int n;
    string oficial;
    try {
        n = validarRejilla(e.rejilla);
        oficial = cifrar(texto, e.rejilla, e.relleno);
    } catch (const invalid_argument& ex) {
        cout << ROJO() << "  " << ex.what() << RST() << "\n";
        return;
    }
    string claro = normalizar(texto);
    size_t tam = (size_t)n * n, largoReal = claro.size();
    claro.append((tam - claro.size() % tam) % tam, (char)toupper((unsigned char)e.relleno));

    // Huecos de cada giro, igual que en cifrar(): ordenados por filas y girando 90 grados.
    vector<vector<Posicion>> giros;
    vector<Posicion> actual = leerHuecos(e.rejilla);
    for (int g = 0; g < 4; g++) {
        sort(actual.begin(), actual.end());
        giros.push_back(actual);
        for (Posicion& p : actual) p = {p.second, n - 1 - p.first};
    }

    size_t bloques = claro.size() / tam, b = 0;
    if (bloques > 1) {
        string s = preguntar("  Hay " + to_string(bloques) + " bloques. Cual quieres ver? [1] ");
        try { if (!s.empty()) b = std::stoul(s) - 1; } catch (...) {}
        if (b >= bloques) b = 0;
    }
    cout << "  Texto: " << claro.substr(0, largoReal) << "\n";

    vector<string> letras(n, string(n, ' '));
    vector<vector<int>> tipo(n, vector<int>(n, 0));
    size_t k = b * tam;
    for (int g = 0; g < 4; g++) {
        for (auto& fila : tipo) for (int& t : fila) if (t == 1) t = 2;
        string escritas;
        for (const Posicion& p : giros[g]) {
            letras[p.first][p.second] = claro[k];
            tipo[p.first][p.second] = 1;
            escritas += claro[k++];
        }
        cout << "\n  " << NEG() << "Giro " << g + 1 << " (" << g * 90 << " grados)" << RST()
             << "  se escriben: " << AZUL() << escritas << RST() << "\n";
        dibujar(n, letras, tipo);
        if (g < 3 && !finEntrada) preguntar("  [Enter: girar la rejilla 90 grados] ");
    }
    k = b * tam;
    for (int g = 0; g < 4; g++)
        for (const Posicion& p : giros[g]) tipo[p.first][p.second] = (k++ >= largoReal) ? 3 : 2;
    cout << "\n  " << NEG() << "Matriz final: se lee por filas" << RST() << (colores ? "  (amarillo = relleno)" : "") << "\n";
    dibujar(n, letras, tipo);
    string bloque;
    for (const string& fila : letras) bloque += fila;
    cout << "\n  Bloque " << b + 1 << " cifrado: " << VERDE() << bloque << RST() << "\n";
    bool coincide = oficial.substr(b * tam, tam) == bloque;
    cout << (coincide ? GRIS() + "  (Coincide con lo que devuelve cifrar())" : ROJO() + "  No coincide con cifrar()")
         << RST() << "\n";
}

// ---------------------------------------------------------------------------
// 3) Descifrar / verificar
// ---------------------------------------------------------------------------
static void accionDescifrar(Estado& e) {
    cout << NEG() << "3. DESCIFRAR / VERIFICAR\n" << RST();
    string cif;
    bool esUltimo = false;
    if (!e.ultimoCifrado.empty()) {
        string s = preguntar("  Descifrar el ultimo cifrado? (S/n) ");
        if (s.empty() || s[0] == 's' || s[0] == 'S') { cif = e.ultimoCifrado; esUltimo = true; }
    }
    if (cif.empty()) {
        cout << "  Pega el texto cifrado:\n";
        cif = leerTextoLargo();
        if (cif.empty()) { cout << "  (Cancelado)\n"; return; }
    }
    int lon = -1;
    string s = esUltimo ? preguntar("  Longitud original [" + to_string(e.ultimaLongitud) + "]: ")
                        : preguntar("  Longitud original (Enter si no la sabes): ");
    if (s.empty() && esUltimo) lon = e.ultimaLongitud;
    try { if (!s.empty()) lon = std::stoi(s); } catch (...) { lon = -1; }
    try {
        string claro = descifrar(cif, e.rejilla, lon);  // funcion de P1
        char r = (char)toupper((unsigned char)e.relleno);
        if (lon < 0) {  // sin longitud: se quita el relleno del final
            size_t fin = claro.find_last_not_of(r);
            claro = (fin == string::npos) ? "" : claro.substr(0, fin + 1);
        }
        cout << "\n  " << VERDE() << "TEXTO CLARO:" << RST() << "\n\n    " << NEG() << claro << RST() << "\n";
        if (lon < 0)
            cout << GRIS() << "\n  (Se quito el relleno '" << r << "' del final. Si el mensaje terminaba en esa letra "
                 << "tambien se borro: por eso conviene usar la longitud original.)" << RST() << "\n";
        if (esUltimo)
            cout << (claro == normalizar(e.ultimoTexto) ? VERDE() + "\n  Coincide con el texto original."
                                                        : ROJO() + "\n  No coincide con el texto original.")
                 << RST() << "\n";
    } catch (const invalid_argument& ex) {
        cout << ROJO() << "\n  No se pudo descifrar: " << ex.what() << RST() << "\n";
    }
}

// ---------------------------------------------------------------------------
// 4) Cambiar rejilla
// ---------------------------------------------------------------------------
static vector<string> leerFilas(const string& t) {
    vector<string> filas;
    string fila;
    for (char c : t) {
        if (c == '/' || c == '\n' || c == ' ') { if (!fila.empty()) { filas.push_back(fila); fila.clear(); } }
        else if (c != '\r') fila += c;
    }
    if (!fila.empty()) filas.push_back(fila);
    return filas;
}

static vector<string> rejillaAlAzar(int n) {
    std::mt19937 rng(std::random_device{}());
    vector<string> r(n, string(n, '.'));
    for (int f = 0; f < n / 2; f++)
        for (int c = 0; c < n / 2; c++) {
            int a = f, b = c;
            for (int g = rng() % 4; g > 0; g--) { int na = b; b = n - 1 - a; a = na; }
            r[a][b] = 'X';
        }
    return r;
}

static void accionRejilla(Estado& e) {
    cout << NEG() << "4. CAMBIAR REJILLA\n" << RST() << "  Actual:\n";
    dibujarRejilla(e.rejilla);
    cout << "\n  1) Escribirla  (X = hueco, . = cerrado; filas separadas por / o una por linea)\n"
         << "  2) Generar una al azar\n"
         << "  3) Ejemplo 4x4\n"
         << "  4) Ejemplo 6x6\n"
         << "  0) Volver\n";
    string op = preguntar("  Opcion: ");
    vector<string> nueva;
    if (op == "1") nueva = leerFilas(leerTextoLargo());
    else if (op == "2") {
        string s = preguntar("  Lado (par, de 2 a 12) [6]: ");
        int n = 6;
        try { if (!s.empty()) n = std::stoi(s); } catch (...) { n = -1; }
        if (n < 2 || n > 12 || n % 2) { cout << ROJO() << "  El lado debe ser par, de 2 a 12." << RST() << "\n"; return; }
        nueva = rejillaAlAzar(n);
    } else if (op == "3") nueva = {"X...", "...X", "..X.", ".X.."};
    else if (op == "4") nueva = leerFilas("....../XXX..X/.X.X../X....X/....../.....X");
    else return;
    try {
        validarRejilla(nueva);  // funcion de P1
        e.rejilla = nueva;
        e.ultimoTexto.clear();
        e.ultimoCifrado.clear();
        e.ultimaLongitud = -1;
        cout << VERDE() << "\n  Rejilla valida:" << RST() << "\n";
        dibujarRejilla(e.rejilla);
        cout << "  Como texto: " << AZUL() << rejillaTexto(e.rejilla) << RST() << "  (pasasela al grupo que descifra)\n";
    } catch (const invalid_argument& ex) {
        cout << ROJO() << "\n  Rejilla rechazada: " << ex.what() << RST() << "\n  Se mantiene la anterior.\n";
    }
}

// ---------------------------------------------------------------------------
// 5) Opciones
// ---------------------------------------------------------------------------
static void accionOpciones(Estado& e) {
    cout << NEG() << "5. OPCIONES\n" << RST()
         << "  1) Relleno: " << (char)toupper((unsigned char)e.relleno) << "\n"
         << "  2) Colores: " << (colores ? "si" : "no") << "\n"
         << "  0) Volver\n";
    string op = preguntar("  Opcion: ");
    if (op == "1") {
        string s = preguntar("  Nuevo relleno (una letra o un digito): ");
        if (s.size() == 1 && isalnum((unsigned char)s[0])) {
            e.relleno = (char)toupper((unsigned char)s[0]);
            e.ultimoCifrado.clear();
            e.ultimoTexto.clear();
            e.ultimaLongitud = -1;
            cout << VERDE() << "  Relleno cambiado a " << e.relleno << RST() << "\n";
        } else {
            cout << ROJO() << "  Debe ser una sola letra (A-Z) o un digito." << RST() << "\n";
        }
        pausa();
    } else if (op == "2") {
        colores = !colores;
    }
}

// ---------------------------------------------------------------------------
// 6) Casos de prueba
// ---------------------------------------------------------------------------
static void accionCasos() {
    cout << NEG() << "6. CASOS DE PRUEBA\n" << RST();
    vector<prueba::Resultado> res = prueba::ejecutarTodos();
    int ok = 0;
    string grupo;
    for (size_t i = 0; i < res.size(); i++) {
        const prueba::Resultado& r = res[i];
        if (r.caso.grupo != grupo) { grupo = r.caso.grupo; cout << "\n  " << NEG() << grupo << RST() << "\n"; }
        ok += r.ok;
        cout << "  " << (r.ok ? VERDE() + "PASA " : ROJO() + "FALLA") << RST() << " " << (i + 1 < 10 ? " " : "")
             << i + 1 << ". " << r.caso.nombre << "\n";
    }
    cout << "\n  " << (ok == (int)res.size() ? VERDE() : ROJO()) << ok << " de " << res.size() << " pasan, "
         << res.size() - ok << " fallan." << RST() << "\n";
    while (!finEntrada) {
        string s = preguntar("\n  Numero de caso para ver el detalle (Enter para salir): ");
        if (s.empty()) break;
        size_t k = 0;
        try { k = std::stoul(s); } catch (...) {}
        if (k < 1 || k > res.size()) continue;
        const prueba::Resultado& r = res[k - 1];
        cout << "\n  " << NEG() << r.caso.nombre << RST() << "  (" << r.caso.grupo << ")\n"
             << "  " << r.caso.porque << "\n"
             << "  Rejilla  : " << (r.caso.rejilla.empty() ? "(vacia)" : r.caso.rejilla) << "\n";
        if (r.caso.esDescifrado) {
            cout << "  Cifrado  : " << (r.caso.cifradoDado.empty() ? "cifrar(\"" + r.caso.texto + "\")" : r.caso.cifradoDado) << "\n"
                 << "  Longitud : " << (r.caso.longitud < 0 ? string("(no se da)") : to_string(r.caso.longitud)) << "\n";
        } else {
            cout << "  Entrada  : " << r.caso.texto << "\n";
            if (!r.caso.esperaError) cout << "  Esperado normalizado: " << r.caso.normalizado << "\n";
        }
        cout << "  Esperado : " << r.esperado << "\n"
             << "  Obtenido : " << (r.obtenido.empty() ? "(vacio)" : r.obtenido) << "\n"
             << "  Resultado: " << (r.ok ? VERDE() + "PASA" : ROJO() + "FALLA") << RST()
             << (r.nota.empty() ? "" : "  - " + r.nota) << "\n";
    }
}

// ---------------------------------------------------------------------------
int main() {
    prepararConsola();
    Estado e;
    while (!finEntrada) {
        limpiar();
        int n = e.rejilla.size();
        cout << NEG() << "==============================================================\n"
             << "   CIFRADO POR REJILLA GIRATORIA  (Fleissner)\n"
             << "==============================================================" << RST() << "\n"
             << "  Rejilla actual " << n << "x" << n << ": " << AZUL() << rejillaTexto(e.rejilla) << RST()
             << "   Giro: horario   Relleno: " << (char)toupper((unsigned char)e.relleno) << "\n\n";
        dibujarRejilla(e.rejilla);
        cout << "\n  1) Cifrar texto\n"
             << "  2) Ver paso a paso el ultimo cifrado\n"
             << "  3) Descifrar / verificar\n"
             << "  4) Cambiar rejilla\n"
             << "  5) Opciones (relleno, colores)\n"
             << "  6) Casos de prueba\n"
             << "  0) Salir\n\n";
        string op = preguntar("  Elige una opcion: ");
        if (finEntrada || op == "0") break;
        cout << "\n";
        if (op == "1") accionCifrar(e);
        else if (op == "2") accionPasoAPaso(e);
        else if (op == "3") accionDescifrar(e);
        else if (op == "4") accionRejilla(e);
        else if (op == "5") { accionOpciones(e); continue; }
        else if (op == "6") accionCasos();
        else continue;
        pausa();
    }
    cout << "\n  Hasta luego.\n";
    return 0;
}
