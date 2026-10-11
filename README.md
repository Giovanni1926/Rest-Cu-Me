# Rest Cu Me

Client desktop per chiamate **REST** e **SOAP** costruite tramite un **flusso visuale a nodi**.

Scrivi una sequenza di chiamate HTTP su un canvas: ogni nodo è una chiamata (REST o SOAP) oppure un insieme di variabili, i collegamenti tra i nodi passano i dati da una risposta all'altra. Salvi il tutto in un file `.rcmflow` e lo esegui con un click.

## In soldoni, cosa fa

- **Editor a nodi**: disegni il flusso con nodi REST, SOAP e Variabili, raggruppabili in **collection**.
- **Esegui tutto / da qui**: il motore pianifica l'ordine, rileva i cicli e sostituisce le variabili `${nome}` tra un nodo e l'altro.
- **Estrazione dati**: prendi valori dalla risposta di un nodo (JSONPath o XPath) e li inietti nel successivo.
- **Ogni chiamata completa**: metodi HTTP, header, query, cookie, body in più formati (JSON/XML/testo/form/multipart), autenticazione (none/Bearer/Basic/API key), timeout e TLS/mTLS.
- **SOAP con import WSDL**: genera l'envelope 1.1/1.2, importa i parametri da un WSDL.
- **Console + Cronologia**: log colorato e storico persistito delle chiamate (richiesta risolta e risposta), con riesegui e copia JSON.
- **Doppia vista per file**: vista **Grafo** (canvas) e vista **Visual** per compilare ed eseguire una singola chiamata.
- **Explorer laterale**: albero dei file aperti → collection → nodi, con selezione nel grafo e creazione rapida via tasto destro.

## Formato file

`.rcmflow` è un JSON semplice e versionato, compatibile con il plugin IntelliJ originale da cui il progetto è nato.

## Requisiti e build

- **C++17**, **Qt 6** (Widgets, Network, Concurrent), **CMake** + Ninja, toolchain MinGW.
- Dipendenze vendored in `third_party/`: `nlohmann/json` (JSON) e `pugixml` (XML/XPath).

```powershell
cmake -G Ninja -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\mingw_64
cmake --build build
# eseguibile: build\RestCuMe.exe
```

## Struttura

```
src/
├── model/    # modelli e parser (chiamate, auth, TLS, multipart, WSDL)
├── http/     # esecuzione HTTP, costruzione body, persistenza cronologia
├── flow/     # grafo, pianificatore, estrazioni JSONPath/XPath, esecuzione
├── run/      # hub console/cronologia
└── ui/       # canvas, dialog, explorer, vista visual, tool window
```
