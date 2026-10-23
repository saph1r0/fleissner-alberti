# Alberti // Fleissner

Criptografía clásica, tarea 3.2. Nuestro grupo **cifra con la rejilla giratoria de Fleissner** y **descifra el disco de Alberti**.

**Página web:** https://saph1r0.github.io/fleissner-alberti/

## La página (`index.html`)
Funciona en el navegador, sin instalar nada.

- **Izquierda, descifrar Alberti:** se pega el mensaje cifrado y la configuración (el JSON de Cifrario o un `config.txt`). Muestra el texto claro y un paso a paso con los dos discos.
- **Derecha, cifrar con rejilla:** se escribe el texto, se elige la rejilla y se obtiene el cifrado más el JSON para el otro grupo. Tiene paso a paso de los 4 giros.
- **Pruebas:** al abrir la página se ejecutan los casos de rejilla (53), de Alberti (29) y de compatibilidad con Cifrario (12).

La lógica es la misma que la de los programas en C++ de este repositorio (comparada con 8000 entradas al azar).

## Programas en C++
Requiere g++ con C++17.

### Rejilla (`rejilla/`)
```sh
cd rejilla
g++ -std=c++17 interfaz.cpp -o rejilla   # interfaz de consola (incluye cifrado_rejillas.cpp)
g++ -std=c++17 pruebas.cpp  -o pruebas   # 53 casos de prueba
```
No compilar `cifrado_rejillas.cpp` junto con los otros: ya se incluye dentro.

### Alberti (`alberti/`)
```sh
cd alberti
g++ -std=c++17 alberti.cpp -o alberti
./alberti            # menú: lee config.txt y mensaje.txt
./alberti --pruebas  # casos de prueba
```

## Convenciones
- **Rejilla:** alfabeto A–Z y 0–9; tildes fuera y Ñ → N; sin espacios ni signos. Giro horario, huecos leídos por filas, relleno `X`. Hay que pasar la longitud original.
- **Alberti:** k(i) = (k0 + ⌊i/L⌋·inc) mod n. Cifrario cifra con D2[pos + k]; para descifrar su mensaje se resta el giro (en `config.txt`: `SENTIDO = -1`).
