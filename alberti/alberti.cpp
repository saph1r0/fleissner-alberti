/*
- Disco de Alberti - Descifrado (P4)
- Compilar: g++ -std=c++17 alberti.cpp -o alberti
- Ejecutar: ./alberti
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <iomanip>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

// ---------- colores ANSI para la terminal ----------
const string C_RESET = "\033[0m";
const string C_VERDE = "\033[32m";
const string C_ROJO = "\033[31m";
const string C_CYAN = "\033[36m";
const string C_AMARILLO = "\033[33m";
const string C_BOLD = "\033[1m";

// ---------- configuracion ----------
struct Config {
    vector<string> fijo;    
    vector<string> movil;   
    int k0 = 0;             
    int inc = 1;            
    int L = 1;              
    bool conservarCaja = true;          
    bool conservarDesconocidos = true;  
    int sentido = 1;                    
};

vector<string> tokenizar(const string& texto) {
    vector<string> t; size_t i = 0;
    while (i < texto.size()) {
        unsigned char ch = texto[i]; size_t len = 1;
        if ((ch & 0x80) == 0x00) len = 1;
        else if ((ch & 0xE0) == 0xC0) len = 2;
        else if ((ch & 0xF0) == 0xE0) len = 3;
        else if ((ch & 0xF8) == 0xF0) len = 4;
        if (i + len > texto.size()) len = texto.size() - i;
        t.push_back(texto.substr(i, len)); i += len;
    }
    return t;
}
string unir(const vector<string>& v) { string s; for (auto& x : v) s += x; return s; }

static string aMay(string t) { if (t.size() == 1) t[0] = toupper((unsigned char)t[0]); return t; }
static string aMin(string t) { if (t.size() == 1) t[0] = tolower((unsigned char)t[0]); return t; }
static bool esMin(const string& t) { return t.size() == 1 && islower((unsigned char)t[0]); }
static bool esMay(const string& t) { return t.size() == 1 && isupper((unsigned char)t[0]); }

string validar(const Config& cfg) {
    if (cfg.fijo.empty() || cfg.movil.empty()) return "Los discos no pueden estar vacios.";
    if (cfg.fijo.size() != cfg.movil.size())
        return "Los discos deben tener la misma longitud.";
    if (cfg.L < 1) return "El periodo (L) debe ser >= 1.";
    unordered_set<string> a, b;
    for (auto& s : cfg.fijo)  if (!a.insert(aMin(s)).second) return "Disco 1 repetido: " + s;
    for (auto& s : cfg.movil) if (!b.insert(aMin(s)).second) return "Disco 2 repetido: " + s;
    return "";
}

static string recortar(string s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == string::npos ? "" : s.substr(a, b - a + 1);
}
static string enMinusculas(string s) { for (auto& ch : s) ch = tolower((unsigned char)ch); return s; }

Config leerConfig(const string& ruta) {
    ifstream f(ruta, ios::binary);
    if (!f) throw runtime_error("No se pudo abrir el archivo de configuracion: " + ruta);
    Config cfg; string linea;
    while (getline(f, linea)) {
        linea = recortar(linea);
        if (linea.empty() || linea[0] == '#') continue;
        size_t p = linea.find('=');
        if (p == string::npos) continue;
        string k = enMinusculas(recortar(linea.substr(0, p))), v = recortar(linea.substr(p + 1));
        if (k == "disco1") cfg.fijo = tokenizar(v);
        else if (k == "disco2") cfg.movil = tokenizar(v);
        else if (k == "giro_inicial") cfg.k0 = stoi(v);
        else if (k == "incremento") cfg.inc = stoi(v);
        else if (k == "periodo") cfg.L = stoi(v);
        else if (k == "mayusculas") cfg.conservarCaja = (enMinusculas(v) != "normalizar");
        else if (k == "desconocidos") cfg.conservarDesconocidos = (enMinusculas(v) != "eliminar");
        else if (k == "sentido") cfg.sentido = (stoi(v) < 0) ? -1 : 1;
    }
    return cfg;
}

string leerArchivo(const string& ruta) {
    ifstream f(ruta, ios::binary);
    if (!f) throw runtime_error("No se pudo abrir el archivo: " + ruta);
    stringstream b; b << f.rdbuf();
    string s = b.str();
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) s = s.substr(3);
    return s;
}

// ---------- nucleo ----------
int desplazamiento(int i, const Config& cfg) {
    long long n = cfg.fijo.size();
    long long k = cfg.k0 + (long long)(i / cfg.L) * cfg.inc;
    return (int)(((k % n) + n) % n);
}

static unordered_map<string, int> indice(const vector<string>& d) {
    unordered_map<string, int> m;
    for (size_t j = 0; j < d.size(); j++) m[aMin(d[j])] = (int)j;
    return m;
}

struct Paso { int i; string cif, claro; int posM, k, posF; };

string descifrar(const string& textoCifrado, const Config& cfg, vector<Paso>* pasos = nullptr) {
    string err = validar(cfg);
    if (!err.empty()) throw invalid_argument(err);
    auto posM = indice(cfg.movil);
    int n = cfg.fijo.size(), i = 0;
    vector<string> sal;
    int cajaPrevia = 1;
    for (const string& tok : tokenizar(textoCifrado)) {
        auto it = posM.find(aMin(tok));
        if (it == posM.end()) { if (cfg.conservarDesconocidos) sal.push_back(tok); continue; }
        int k = desplazamiento(i, cfg);
        int pf = (((it->second + cfg.sentido * k) % n) + n) % n;
        string r = cfg.fijo[pf];
        if (!cfg.conservarCaja) r = aMay(r);
        else if (esMay(tok)) { r = aMay(r); cajaPrevia = 1; }
        else if (esMin(tok)) { r = aMin(r); cajaPrevia = 0; }
        else r = cajaPrevia ? aMay(r) : aMin(r);
        if (pasos) pasos->push_back({i, tok, r, it->second, k, pf});
        sal.push_back(r); i++;
    }
    return unir(sal);
}

string cifrar(const string& texto, const Config& cfg) {
    string err = validar(cfg);
    if (!err.empty()) throw invalid_argument(err);
    auto posF = indice(cfg.fijo);
    int n = cfg.fijo.size(), i = 0;
    vector<string> sal;
    for (const string& tok : tokenizar(texto)) {
        auto it = posF.find(aMin(tok));
        if (it == posF.end()) { if (cfg.conservarDesconocidos) sal.push_back(tok); continue; }
        int k = desplazamiento(i, cfg);
        int q = (((it->second - cfg.sentido * k) % n) + n) % n;
        string r = cfg.movil[q];
        if (cfg.conservarCaja) { if (esMay(tok)) r = aMay(r); else if (esMin(tok)) r = aMin(r); }
        sal.push_back(r); i++;
    }
    return unir(sal);
}

// ---------- dibujo de los discos ----------
void dibujarDiscos(const Config& cfg, int k, int resaltar) {
    int n = cfg.fijo.size();
    cout << C_CYAN << "  Disco 1 (fijo)  : " << C_RESET;
    for (int p = 0; p < n; p++) cout << (p == resaltar ? C_AMARILLO + "[" + aMay(cfg.fijo[p]) + "]" + C_RESET : cfg.fijo[p]) << ' ';
    cout << "\n" << C_CYAN << "  Disco 2 (gira " << k << "): " << C_RESET;
    for (int p = 0; p < n; p++) {
        int j = (((p - cfg.sentido * k) % n) + n) % n;
        cout << (p == resaltar ? C_AMARILLO + "[" + cfg.movil[j] + "]" + C_RESET : cfg.movil[j]) << ' ';
    }
    cout << "\n";
}

void verPasoAPaso(const string& cifrado, const Config& cfg) {
    vector<Paso> pasos;
    string res = descifrar(cifrado, cfg, &pasos);
    cout << "\n" << C_BOLD << C_CYAN << "=== PASO A PASO ===" << C_RESET << "\n";
    cout << setw(6) << "i" << setw(10) << "cifrada" << setw(10) << "pos D2" << setw(8) << "k(i)"
         << setw(10) << "pos D1" << setw(8) << "claro" << "\n";
    cout << string(52, '-') << "\n";
    int ultimoK = -1, mostrados = 0;
    for (auto& p : pasos) {
        if (mostrados++ >= 60) { cout << " ... (primeros 60)\n"; break; }
        cout << setw(6) << p.i << setw(10) << p.cif << setw(10) << p.posM << setw(8) << p.k
             << setw(10) << p.posF << setw(10) << p.claro << "\n";
    }
    cout << "\n" << C_CYAN << "Alineacion de discos:" << C_RESET << "\n";
    for (auto& p : pasos) {
        if (p.k != ultimoK && p.i % cfg.L == 0) {
            cout << "\n i=" << p.i << " k=" << p.k << " ('" << p.cif << "' -> '" << p.claro << "')\n";
            dibujarDiscos(cfg, p.k, p.posF);
            ultimoK = p.k;
            if (p.i / cfg.L >= 3) { cout << " ...\n"; break; }
        }
    }
    cout << "\n" << C_BOLD << C_VERDE << "Mensaje descifrado:" << C_RESET << "\n" << res << "\n";
}

int correrPruebas(bool detalle) {
    int ok = 0, total = 0;
    auto igualCaja = [](string a, string b) { return enMinusculas(a) == enMinusculas(b); };
    
    auto reporta = [&](const string& nombre, const string& original, const string& cifrado, const string& descifrado, bool pasa) {
        total++; if (pasa) ok++;
        if (detalle) {
            cout << (pasa ? C_VERDE + " [OK]   " + C_RESET : C_ROJO + " [FALLA]" + C_RESET) << " " << nombre << "\n";
            cout << "          Original   : \"" << original << "\"\n";
            cout << "          Cifrado    : \"" << cifrado << "\"\n";
            cout << "          Descifrado : \"" << descifrado << "\"\n";
        }
    };

    Config base;
    base.fijo = tokenizar("ABCDEFGILMNOPQRSTVXZ1234");
    base.movil = tokenizar("usqomkhfdbacegilnprtxz&y");
    base.k0 = 0; base.inc = 1; base.L = 4;

    // 1) Valor conocido
    {
        string original = "AMANECER";
        string cif = cifrar(original, base);
        string dec = descifrar(cif, base);
        reporta("1. Valor conocido", original, cif, dec, dec == original);
    }

    // 2-13) Textos variados
    vector<pair<string, string>> textos = {
        {"Texto corto", "SALDO"},
        {"Con espacios", "ATACAR AL AMANECER"},
        {"Minusculas", "atacar al amanecer"},
        {"Mayusculas y minusculas", "Atacar Al Amanecer"},
        {"Con tildes y enie", "El senor Onoro comio pan en Panama"},
        {"Con tildes reales", "El se\xC3\xB1or \xC3\x91o\xC3\xB1o comi\xC3\xB3 pan en Panam\xC3\xA1"},
        {"Signos y saltos de linea", "\xC2\xBF" "Atacar? \xC2\xA1" "Si! (oso)\nlinea 2"},
        {"Con digitos 1-4", "CLAVE 1234 E 99"},
        {"Un solo caracter", "A"},
        {"Solo simbolos ajenos", "\xC2\xBF?!, .."},
        {"Cruza varios periodos", "ABCDEFGILMNOPQRSTVXZ1234ABCDEFGILMNOPQRSTVXZ1234"},
        {"Mas de una vuelta", string(30, 'A')}
    };
    int idx = 2;
    for (auto& t : textos) {
        Config c = base;
        string original = t.second;
        string cif = cifrar(original, c);
        string dec = descifrar(cif, c);
        reporta(to_string(idx++) + ". " + t.first, original, cif, dec, igualCaja(dec, original));
    }

    // 14-16) Opciones
    { 
        Config c = base; c.conservarCaja = false;
        string original = "Atacar al amanecer";
        string cif = cifrar(original, c);
        string dec = descifrar(cif, c);
        reporta("15. Opcion: Normalizar a mayusculas", original, cif, dec, dec == "ATACAR AL AMANECER"); 
    }
    { 
        Config c = base; c.conservarDesconocidos = false;
        string original = "Atacar, al amanecer!";
        string cif = cifrar(original, c);
        string dec = descifrar(cif, c);
        reporta("16. Opcion: Eliminar simbolos ajenos", original, cif, dec, igualCaja(dec, "Atacaralamanecer")); 
    }
    { 
        Config c = base; c.sentido = -1; 
        string original = "Atacar al amanecer";
        string cif = cifrar(original, c);
        string dec = descifrar(cif, c);
        reporta("17. Opcion: Sentido inverso (sentido = -1)", original, cif, dec, igualCaja(dec, original)); 
    }

    // 18-23) Parámetros distintos
    struct Param { int k0, inc, L; };
    vector<Param> params = {{0,0,1}, {5,3,2}, {23,1,24}, {-3,-2,5}, {100,7,3}, {0,1,1}};
    int pIdx = 18;
    for (auto& p : params) {
        Config c = base; c.k0 = p.k0; c.inc = p.inc; c.L = p.L;
        string original = "Atacar al amanecer";
        string cif = cifrar(original, c);
        string dec = descifrar(cif, c);
        reporta(to_string(pIdx++) + ". Parametros (k0=" + to_string(p.k0) + ", inc=" + to_string(p.inc) + ", L=" + to_string(p.L) + ")", original, cif, dec, igualCaja(dec, original));
    }

    // 24) Disco raro
    { 
        Config c; c.fijo = tokenizar("ABC\xC3\x91"); c.movil = tokenizar("xyz&"); c.L = 2; c.inc = 1;
        string original = "A\xC3\x91" "CBA"; 
        string cif = cifrar(original, c);
        string dec = descifrar(cif, c);
        reporta("24. Disco raro con enie (UTF-8)", original, cif, dec, dec == original); 
    }

    // 25-30) Deben fallar
    auto debeFallar = [&](int num, const string& nombre, Config c) {
        bool fallo = false; string errMsg = "";
        try { descifrar("abc", c); } catch (const exception& e) { fallo = true; errMsg = e.what(); }
        reporta(to_string(num) + ". Debe fallar: " + nombre, "[Configuracion invalida]", "N/A", "Error capturado: " + errMsg, fallo);
    };
    { Config c = base; c.movil.pop_back(); debeFallar(25, "Longitudes distintas", c); }
    { Config c = base; c.movil[1] = c.movil[0]; debeFallar(26, "Simbolo repetido en disco 2", c); }
    { Config c = base; c.fijo[1] = c.fijo[0]; debeFallar(27, "Simbolo repetido en disco 1", c); }
    { Config c = base; c.L = 0; debeFallar(28, "Periodo 0", c); }
    { Config c = base; c.fijo.clear(); c.movil.clear(); debeFallar(29, "Discos vacios", c); }
    { Config c; c.fijo = tokenizar("ABC"); c.movil = tokenizar("aB c"); debeFallar(30, "Simbolos repetidos por caja", c); }

    cout << "\n========================================\n";
    cout << C_BOLD << (ok == total ? C_VERDE : C_ROJO) << " RESULTADO FINAL: " << ok << " de " << total << " casos exitosos." << C_RESET << "\n";
    cout << "========================================\n";
    return ok == total ? 0 : 1;
}

// ---------- interfaz ----------
static void configInfo(const Config& cfg) {
    cout << "\n" << C_VERDE << "Configuracion cargada correctamente:" << C_RESET << "\n";
    cout << "  Disco 1 (fijo)   : " << unir(cfg.fijo) << " (" << cfg.fijo.size() << " simbolos)\n";
    cout << "  Disco 2 (movil)  : " << unir(cfg.movil) << " (" << cfg.movil.size() << " simbolos)\n";
    cout << "  Giro inicial=" << cfg.k0 << "  Incremento=" << cfg.inc << "  Periodo=" << cfg.L << "\n";
    string e = validar(cfg);
    cout << "  Validacion       : " << (e.empty() ? C_VERDE + "Correcta" + C_RESET : C_ROJO + e + C_RESET) << "\n";
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    if (argc > 1 && string(argv[1]) == "--pruebas") return correrPruebas(true);

    Config cfg; bool hayCfg = false; string cifrado; bool hayMsg = false;
    while (true) {
        cout << "\n" << C_BOLD << C_CYAN << "=== DISCO DE ALBERTI - DESCIFRADO ===" << C_RESET << "\n"
             << " 1) Cargar configuracion (config.txt)\n"
             << " 2) Cargar mensaje cifrado (mensaje.txt)\n"
             << " 3) Descifrar\n"
             << " 4) Ver paso a paso (incluye discos)\n"
             << " 5) Casos de prueba \n"
             << " 0) Salir\n"
             << C_AMARILLO << "Elige una opcion: " << C_RESET;
        string op; if (!getline(cin, op)) break; op = recortar(op);
        try {
            if (op == "0") break;
            else if (op == "1") { cfg = leerConfig("config.txt"); hayCfg = true; configInfo(cfg); }
            else if (op == "2") { cifrado = leerArchivo("mensaje.txt"); hayMsg = true;
                                  cout << C_VERDE << "Mensaje cargado (" << tokenizar(cifrado).size() << " caracteres)." << C_RESET << "\n"; }
            else if (op == "3" || op == "4") {
                if (!hayCfg) throw runtime_error("Primero carga la configuracion (opcion 1).");
                if (!hayMsg) throw runtime_error("Primero carga el mensaje (opcion 2).");
                if (op == "3") {
                    string res = descifrar(cifrado, cfg);
                    cout << "\n" << C_BOLD << C_VERDE << "Mensaje descifrado:" << C_RESET << "\n" << res << "\n";
                    ofstream("descifrado.txt", ios::binary) << res;
                    cout << C_CYAN << "(Guardado en descifrado.txt)" << C_RESET << "\n";
                } else {
                    verPasoAPaso(cifrado, cfg);
                }
            }
            else if (op == "5") correrPruebas(true);
            else cout << C_ROJO << "Opcion no valida." << C_RESET << "\n";
        } catch (const exception& e) { cerr << C_ROJO << "Error: " << e.what() << C_RESET << "\n"; }
    }
    return 0;
}