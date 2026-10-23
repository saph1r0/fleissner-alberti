/*
 * casos_prueba.hpp - Casos de prueba ESTRICTOS para el codigo de P1 (P2: tester)
 *
 * Prueba cifrar() y descifrar() de cifrado_rejillas.cpp.
 * Este archivo se incluye DESPUES de cifrado_rejillas.cpp (ver pruebas.cpp).
 *
 * Como se decide si un caso pasa:
 *   - Casos normales: para cada texto se escribio A MANO el texto normalizado
 *     esperado (mayusculas, sin tildes, espacios ni signos, como dice el
 *     comentario de P1). El cifrado esperado se calcula con un cifrador de
 *     referencia independiente (gira la plantilla en vez de las posiciones).
 *     PASA solo si cifrar() devuelve exactamente ese cifrado.
 *   - Casos de error: PASA solo si cifrar() lanza invalid_argument.
 *     Devolver un texto vacio u otro resultado cuenta como FALLA.
 *   - Casos de descifrar: PASA si descifrar() devuelve exactamente el texto
 *     esperado (o lanza error cuando corresponde).
 */
#ifndef CASOS_PRUEBA_HPP
#define CASOS_PRUEBA_HPP

#include <stdexcept>
#include <string>
#include <vector>

namespace prueba {
using std::string;
using std::vector;

// ---------------------------------------------------------------------------
// Cifrador de referencia (independiente del de P1)
// ---------------------------------------------------------------------------
// Gira la PLANTILLA completa 90 grados en sentido horario.
inline vector<string> girarPlantilla(const vector<string>& r) {
    int n = r.size();
    vector<string> g(n, string(n, '.'));
    for (int f = 0; f < n; f++)
        for (int c = 0; c < n; c++) g[c][n - 1 - f] = r[f][c];
    return g;
}

// Cifra un texto YA normalizado (solo trasposicion).
inline string cifrarReferencia(string claro, const vector<string>& rejilla, char relleno = 'X') {
    int n = rejilla.size();
    size_t tam = (size_t)n * n;
    while (claro.size() % tam) claro += relleno;
    // Orden de escritura: en cada giro, recorrer la plantilla por filas.
    vector<int> orden;
    vector<string> p = rejilla;
    for (int g = 0; g < 4; g++) {
        for (int f = 0; f < n; f++)
            for (int c = 0; c < n; c++)
                if (p[f][c] == 'X' || p[f][c] == 'x') orden.push_back(f * n + c);
        p = girarPlantilla(p);
    }
    string salida;
    for (size_t b = 0; b < claro.size(); b += tam) {
        string cuadro(tam, '?');
        for (size_t k = 0; k < tam; k++) cuadro[orden[k]] = claro[b + k];
        salida += cuadro;
    }
    return salida;
}

// Descifrador de referencia: para comprobar que el cifrado vuelve al texto.
inline string descifrarReferencia(const string& cif, const vector<string>& rejilla) {
    int n = rejilla.size();
    size_t tam = (size_t)n * n;
    vector<int> orden;
    vector<string> p = rejilla;
    for (int g = 0; g < 4; g++) {
        for (int f = 0; f < n; f++)
            for (int c = 0; c < n; c++)
                if (p[f][c] == 'X' || p[f][c] == 'x') orden.push_back(f * n + c);
        p = girarPlantilla(p);
    }
    string out;
    for (size_t b = 0; b + tam <= cif.size(); b += tam)
        for (size_t k = 0; k < tam; k++) out += cif[b + orden[k]];
    return out;
}

inline vector<string> partir(const string& s) {  // "X.../...X" -> {"X...","...X"}
    vector<string> v;
    string fila;
    for (char c : s) {
        if (c == '/') { v.push_back(fila); fila.clear(); }
        else fila += c;
    }
    if (!fila.empty() || !v.empty()) v.push_back(fila);
    return v;
}

// ---------------------------------------------------------------------------
// Casos
// ---------------------------------------------------------------------------
struct Caso {
    string grupo;
    string nombre;
    string texto;           // lo que se le pasa a cifrar()
    string rejilla;         // filas separadas por '/'
    char relleno;
    string normalizado;     // escrito a mano ("" en casos de error)
    bool esperaError;
    string porque;
    string cifradoFijo;     // opcional: cifrado conocido de antemano
    // Solo para casos de descifrar():
    bool esDescifrado = false;
    string cifradoDado;     // si esta vacio, se usa cifrar(texto)
    int longitud = -1;      // longitud original que se pasa a descifrar()
};

struct Resultado {
    Caso caso;
    bool ok = false;
    string esperado;        // cifrado esperado o "ERROR"
    string obtenido;        // cifrado obtenido o "ERROR: ..."
    string nota;
};

inline vector<Caso> casos() {
    const string R4 = "X.../...X/..X./.X..";
    const string R6 = "....../XXX..X/.X.X../X....X/....../.....X";
    const string R8 = "...X..XX/...X..X./X.....X./XX...X../....XX../..X...../..X...../.....XX.";
    const string LARGO =
        "La criptografia clasica usaba trasposiciones como la rejilla de Fleissner y "
        "sustituciones como el disco de Alberti; hoy se estudian para entender por que "
        "los cifrados modernos son como son.";
    const string LARGO_N =
        "LACRIPTOGRAFIACLASICAUSABATRASPOSICIONESCOMOLAREJILLADEFLEISSNERY"
        "SUSTITUCIONESCOMOELDISCODEALBERTIHOYSEESTUDIANPARAENTENDERPORQUE"
        "LOSCIFRADOSMODERNOSSONCOMOSON";
    vector<Caso> v = {
        // ---------------- Pedidos en el trabajo ----------------
        {"Pedidos", "Texto corto", "HOLA", R4, 'X', "HOLA", false,
         "4 letras en 16 casillas: 12 de relleno.", "HXXXXXXOXXLXXAXX"},
        {"Pedidos", "Texto largo (5 bloques)", LARGO, R6, 'X', LARGO_N, false,
         "158 letras: 5 bloques de 6x6 con la misma rejilla.", ""},
        {"Pedidos", "Con espacios", "Ataque al amanecer", R4, 'X', "ATAQUEALAMANECER", false,
         "Los espacios se eliminan.", "AEAUEMCTAAAERQLN"},
        {"Pedidos", "Con tildes y \xC3\xB1", "La se\xC3\xB1ora N\xC3\xBA\xC3\xB1" "ez tom\xC3\xB3 caf\xC3\xA9 en Per\xC3\xBA", R6, 'X',
         "LASENORANUNEZTOMOCAFEENPERU", false, "\xC3\xA1 \xC3\xA9 \xC3\xB3 \xC3\xBA -> sin tilde, \xC3\xB1 -> N.", ""},
        {"Pedidos", "May\xC3\xBA" "sculas con tilde", "\xC3\x81RBOL \xC3\x89XITO \xC3\x8D" "NDICE \xC3\x93RBITA \xC3\x9A" "NICO \xC3\x91", R6, 'X',
         "ARBOLEXITOINDICEORBITAUNICON", false, "\xC3\x81 \xC3\x89 \xC3\x8D \xC3\x93 \xC3\x9A \xC3\x91 en may\xC3\xBA" "scula.", ""},
        {"Pedidos", "Di\xC3\xA9resis", "Ping\xC3\xBCino, cig\xC3\xBC" "e\xC3\xB1" "a", R4, 'X', "PINGUINOCIGUENA", false,
         "\xC3\xBC -> U.", ""},
        {"Pedidos", "No llena la rejilla", "CIFRADO", R6, 'X', "CIFRADO", false,
         "7 de 36 casillas: 29 de relleno.", ""},
        // ---------------- Bordes ----------------
        {"Bordes", "Llena la rejilla exacto", "CRIPTOGRAFIA2026", R4, 'X', "CRIPTOGRAFIA2026", false,
         "16 caracteres: sin relleno.", ""},
        {"Bordes", "Dos bloques exactos", "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345", R4, 'X',
         "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345", false, "32 caracteres = 2 bloques.", ""},
        {"Bordes", "Un bloque y una letra", "ABCDEFGHIJKLMNOPQ", R4, 'X', "ABCDEFGHIJKLMNOPQ", false,
         "17 caracteres: el segundo bloque lleva 15 de relleno.", ""},
        {"Bordes", "Un solo car\xC3\xA1" "cter", "a", R4, 'X', "A", false, "El m\xC3\xADnimo posible.", ""},
        {"Bordes", "Termina en X", "RELAX", R4, 'X', "RELAX", false,
         "Cifra bien, pero la X final no se distingue del relleno: hay que avisar la longitud (5).", ""},
        {"Bordes", "Relleno en min\xC3\xBA" "scula", "HOLA", R4, 'q', "HOLA", false, "'q' se usa como Q.", ""},
        {"Bordes", "Solo n\xC3\xBA" "meros", "2026 10 20", R4, 'X', "20261020", false, "Los d\xC3\xADgitos se conservan.", ""},
        // ---------------- Texto raro ----------------
        {"Texto raro", "Signos del espa\xC3\xB1ol", "\xC2\xBFHola? \xC2\xA1S\xC3\xAD! (ok) #1", R4, 'X', "HOLASIOK1", false,
         "\xC2\xBF \xC2\xA1 ? ! ( ) # se eliminan.", ""},
        {"Texto raro", "Saltos de l\xC3\xADnea y tabulaciones", "Primera\nSegunda\r\n\tTercera", R6, 'X',
         "PRIMERASEGUNDATERCERA", false, "Se eliminan como los espacios.", ""},
        {"Texto raro", "Tilde separada (Word/Mac)", "Peru\xCC\x81 Jose\xCC\x81", R4, 'X', "PERUJOSE", false,
         "u + tilde suelta: queda la letra.", ""},
        {"Texto raro", "Emoji dentro del texto", "Te veo a las 7 \xF0\x9F\x98\x80", R6, 'X', "TEVEOALAS7", false,
         "El emoji se elimina como un signo.", ""},
        {"Texto raro", "Comillas y guiones tipogr\xC3\xA1" "ficos", "\xE2\x80\x9CHola\xE2\x80\x9D \xE2\x80\x94 ok", R4, 'X',
         "HOLAOK", false, "Comillas curvas y raya de Word.", ""},
        {"Texto raro", "Otras letras con tilde (\xC3\xA3)", "S\xC3\xA3o Paulo", R4, 'X', "SAOPAULO", false,
         "\xC3\xA3 tambi\xC3\xA9n es una letra con tilde: deber\xC3\xAD" "a quedar A.", ""},
        {"Texto raro", "Letras \xC3\xB6 y \xC3\xA7", "G\xC3\xB6ttingen, Fran\xC3\xA7ois", R6, 'X', "GOTTINGENFRANCOIS", false,
         "\xC3\xB6 -> O, \xC3\xA7 -> C.", ""},
        {"Texto raro", "Texto en ANSI (Bloc de notas / consola)", "Per\xFA y Espa\xF1" "a", R4, 'X', "PERUYESPANA", false,
         "Archivo guardado en ANSI: \xC3\xBA y \xC3\xB1 vienen en 1 byte.", ""},
        {"Texto raro", "Franc\xC3\xA9s (\xC3\xA8 \xC3\xBB \xC3\xA0 \xC3\xA7)", "Cr\xC3\xA8me br\xC3\xBBl\xC3\xA9" "e \xC3\xA0 la fran\xC3\xA7" "aise", R6, 'X',
         "CREMEBRULEEALAFRANCAISE", false, "Acentos grave y circunflejo.", ""},
        // ---------------- Rejillas ----------------
        {"Rejillas", "Rejilla m\xC3\xADnima 2x2", "SOS", "X./..", 'X', "SOS", false, "Un hueco, 4 giros.", ""},
        {"Rejillas", "Rejilla 6x6", "Rejillas giratorias de Fleissner", R6, 'X', "REJILLASGIRATORIASDEFLEISSNER", false,
         "9 huecos.", ""},
        {"Rejillas", "Rejilla 8x8", LARGO, R8, 'X', LARGO_N, false, "16 huecos, 3 bloques.", ""},
        {"Rejillas", "Rejilla con x min\xC3\xBA" "scula", "HOLA MUNDO", "x.../...x/..x./.x..", 'X', "HOLAMUNDO", false,
         "x y X valen igual.", ""},
        // ---------------- Deben dar error ----------------
        {"Deben dar error", "Texto vac\xC3\xADo", "", R4, 'X', "", true,
         "No hay nada que cifrar: deber\xC3\xAD" "a avisar.", ""},
        {"Deben dar error", "Solo signos y espacios", "  \xC2\xBF?  \xC2\xA1! ...", R4, 'X', "", true,
         "No queda ninguna letra: deber\xC3\xAD" "a avisar.", ""},
        {"Deben dar error", "Solo emoji", "\xF0\x9F\x98\x80\xF0\x9F\x98\x80", R4, 'X', "", true,
         "No queda ninguna letra: deber\xC3\xAD" "a avisar.", ""},
        {"Deben dar error", "Huecos que se pisan", "HOLA", "X..X/..../..../X..X", 'X', "", true,
         "Las 4 esquinas se descubren 4 veces.", ""},
        {"Deben dar error", "Faltan huecos", "HOLA", "X.../..../..X./....", 'X', "", true,
         "Quedan casillas sin descubrir.", ""},
        {"Deben dar error", "Sobran huecos", "HOLA", "XX../...X/..X./.X..", 'X', "", true, "5 huecos en 4x4.", ""},
        {"Deben dar error", "Rejilla no cuadrada", "HOLA", "X.../...X/..X.", 'X', "", true, "3 filas de 4.", ""},
        {"Deben dar error", "Filas de distinto largo", "HOLA", "X.../...X/..X/.X..", 'X', "", true, "Una fila de 3.", ""},
        {"Deben dar error", "Lado impar", "HOLA", "X../.../...", 'X', "", true, "Solo lado par.", ""},
        {"Deben dar error", "Car\xC3\xA1" "cter no v\xC3\xA1lido", "HOLA", "XA../...X/..X./.X..", 'X', "", true,
         "Solo X y punto.", ""},
        {"Deben dar error", "Rejilla con 1 y 0", "HOLA", "1000/0001/0010/0100", 'X', "", true,
         "El formato acordado es X y punto.", ""},
        {"Deben dar error", "Rejilla vac\xC3\xAD" "a", "HOLA", "", 'X', "", true, "Sin rejilla.", ""},
        {"Deben dar error", "Relleno no v\xC3\xA1lido", "HOLA", R4, '#', "", true, "El relleno debe ser letra o d\xC3\xADgito.", ""},
        {"Deben dar error", "Relleno espacio", "HOLA", R4, ' ', "", true, "El relleno debe ser letra o d\xC3\xADgito.", ""},
    };

    // ---------------- Descifrar ----------------
    auto D = [](string nombre, string texto, string cifradoDado, string rejilla, int longitud,
                string esperado, bool error, string porque) {
        Caso c{"Descifrar", nombre, texto, rejilla, 'X', esperado, error, porque, "", true, "", -1};
        c.esDescifrado = true;
        c.cifradoDado = cifradoDado;
        c.longitud = longitud;
        return c;
    };
    v.push_back(D("Ida y vuelta: texto corto", "HOLA", "", R4, 4, "HOLA", false,
                  "cifrar y luego descifrar con la longitud original."));
    v.push_back(D("Ida y vuelta: texto largo", LARGO, "", R6, 158, LARGO_N, false, "5 bloques de 6x6."));
    v.push_back(D("Ida y vuelta: con tildes", "La se\xC3\xB1ora N\xC3\xBA\xC3\xB1" "ez tom\xC3\xB3 caf\xC3\xA9", "", R4, 21,
                  "LASENORANUNEZTOMOCAFE", false, "2 bloques."));
    v.push_back(D("Ida y vuelta: termina en X", "RELAX", "", R4, 5, "RELAX", false,
                  "Con la longitud, la X del mensaje no se confunde con el relleno."));
    v.push_back(D("Sin longitud: deja el relleno", "HOLA", "", R4, -1, "HOLAXXXXXXXXXXXX", false,
                  "Sin longitud devuelve el bloque completo."));
    v.push_back(D("Cifrado del informe", "", "AEAUEMCTAAAERQLN", R4, -1, "ATAQUEALAMANECER", false,
                  "Cifrado conocido de antemano."));
    v.push_back(D("Cifrado con espacios al copiar", "", " AEAU EMCT\nAAAE RQLN ", R4, -1, "ATAQUEALAMANECER", false,
                  "Se ignoran espacios y saltos de l\xC3\xADnea."));
    v.push_back(D("Cifrado en min\xC3\xBA" "sculas", "", "aeauemctaaaerqln", R4, -1, "ATAQUEALAMANECER", false,
                  "Se pasa a may\xC3\xBA" "sculas."));
    v.push_back(D("ERROR: cifrado incompleto", "", "AEAUEMCTAAAERQL", R4, -1, "", true, "15 caracteres en vez de 16."));
    v.push_back(D("ERROR: cifrado vac\xC3\xADo", "", "   ", R4, -1, "", true, "Nada que descifrar."));
    v.push_back(D("ERROR: longitud mayor", "", "AEAUEMCTAAAERQLN", R4, 20, "", true, "20 > 16."));
    v.push_back(D("ERROR: rejilla inv\xC3\xA1lida", "", "AEAUEMCTAAAERQLN", "X..X/..../..../X..X", -1, "", true,
                  "Se valida la rejilla antes de descifrar."));
    return v;
}

// cifrar() es la funcion de P1 (definida en cifrado_rejillas.cpp).
inline Resultado ejecutarDescifrado(const Caso& c) {
    Resultado r;
    r.caso = c;
    vector<string> rej = partir(c.rejilla);
    r.esperado = c.esperaError ? "ERROR" : c.normalizado;
    try {
        string cif = c.cifradoDado.empty() ? cifrar(c.texto, rej, c.relleno) : c.cifradoDado;
        r.obtenido = descifrar(cif, rej, c.longitud);
        r.ok = !c.esperaError && r.obtenido == r.esperado;
        if (c.esperaError) r.nota = "Devolvi\xC3\xB3 un texto en vez de un error.";
        else if (!r.ok) r.nota = "El descifrado no coincide.";
    } catch (const std::invalid_argument& e) {
        r.obtenido = string("ERROR: ") + e.what();
        r.ok = c.esperaError;
        if (!r.ok) r.nota = "Lanz\xC3\xB3 un error con una entrada v\xC3\xA1lida.";
    }
    return r;
}

inline Resultado ejecutar(const Caso& c) {
    if (c.esDescifrado) return ejecutarDescifrado(c);
    Resultado r;
    r.caso = c;
    vector<string> rej = partir(c.rejilla);
    if (c.esperaError) r.esperado = "ERROR";
    else {
        r.esperado = cifrarReferencia(c.normalizado, rej, (char)toupper((unsigned char)c.relleno));
        if (!c.cifradoFijo.empty() && c.cifradoFijo != r.esperado) r.nota = "(referencia inconsistente)";
    }
    try {
        r.obtenido = cifrar(c.texto, rej, c.relleno);
        if (c.esperaError) {
            r.ok = false;
            r.nota = r.obtenido.empty() ? "Devolvi\xC3\xB3 un cifrado VAC\xC3\x8DO sin avisar."
                                        : "Devolvi\xC3\xB3 un cifrado en vez de un error.";
        } else {
            r.ok = (r.obtenido == r.esperado);
            if (!r.ok) {
                // Ver que texto cifro realmente (descifrando su salida)
                string loQueCifro = descifrarReferencia(r.obtenido, rej);
                size_t fin = loQueCifro.find_last_not_of((char)toupper((unsigned char)c.relleno));
                loQueCifro = (fin == string::npos) ? "" : loQueCifro.substr(0, fin + 1);
                r.nota = "Cifr\xC3\xB3 \"" + loQueCifro + "\" en vez de \"" + c.normalizado + "\".";
            }
        }
    } catch (const std::invalid_argument& e) {
        r.obtenido = string("ERROR: ") + e.what();
        r.ok = c.esperaError;
        if (!r.ok) r.nota = "Lanz\xC3\xB3 un error con un texto v\xC3\xA1lido.";
    } catch (const std::exception& e) {
        r.obtenido = string("EXCEPCION: ") + e.what();
        r.ok = false;
        r.nota = "Excepci\xC3\xB3n inesperada.";
    }
    return r;
}

inline vector<Resultado> ejecutarTodos() {
    vector<Resultado> v;
    for (const Caso& c : casos()) v.push_back(ejecutar(c));
    return v;
}

}  // namespace prueba

#endif
