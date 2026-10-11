# Rest Cu Me — Elenco funzionalità

> Applicazione desktop (C++ / Qt Widgets) per chiamate REST e SOAP tramite flusso visuale a nodi, formato `.rcmflow`.

## Editor visuale a nodi

- Canvas con pan (trascina area vuota) e zoom (Ctrl + rotellina, ancorato al cursore).
- Auto-fit della vista all'apertura del file.
- Griglia di ancoraggio (snap a 10 px).
- Nodi **REST**, **SOAP** e **Variabili**.
- **Collection** (riquadri) che raggruppano i nodi, con drag dell'header (sposta i membri), resize dall'angolo, membership automatica trascinando i nodi dentro/fuori.
- **Link** tra nodi con etichette, wizard di creazione/modifica, estrazioni multiple per arco.
- Selezione e selezione corretta dei nodi sovrapposti (z-order).
- Tasto destro → menu contestuali (nuovo nodo, esegui, duplica, elimina, variabili…).
- Scorciatoie: `Canc`/`Backspace` elimina, `Ctrl+D` duplica.
- Barra strumenti: **Variabili** (globali) e **Cronologia**.
- Autosalvataggio automatico (2 s) e salvataggio manuale.

## Esecuzione del flusso

- Pianificazione DAG: rilevamento cicli, ordine topologico.
- "Esegui tutto" e "Esegui da qui" (Ctrl+Shift+Enter), con inclusione automatica dei nodi variabili a monte.
- Sostituzione `${variabile}` ricorsiva (guardia anti-ciclo, profondità max 20), su URL, query, header, cookie, auth, body, Content-Type, TLS e percorsi multipart.
- Precedenza variabili: output dei link > variabili di collection > variabili globali.
- Colorazione dei nodi: verde (successo), rosso (errore), celeste (SOAP), viola (variabili).

## Chiamate REST

- Metodi: GET, POST, PUT, PATCH, DELETE.
- Query parameters, header, cookie.
- Formati body: **none / JSON / XML / testo / form (x-www-form-urlencoded) / multipart**.
- Multipart con parti testo e file (selezione file, Content-Type per parte, snapshot testuale in console/cronologia).
- Autenticazione: none, Bearer, Basic, API key (header o query).
- Timeout connessione/risposta configurabili (default 30 s / 60 s).
- TLS configurabile: verifica di sistema, accetta tutto (insecure), CA personalizzata, client certificate (mTLS) con chiave PKCS#8.
- User-Agent `Rest-Cu-Me/1.0` se non specificato; HTTP/1.1 forzato.

## Chiamate SOAP

- Envelope SOAP 1.1 / 1.2 generato automaticamente.
- Operazione, namespace, parametri, header SOAP, SOAPAction (derivabile da namespace + operazione).
- Elementi qualificati (`xmlns` di default) o prefisso `tns` (per .NET/ASMX con `elementFormDefault=qualified`).
- XML avanzato (sostituisce l'intero envelope).
- Import **WSDL 1.1** (file o URL): servizi/porte, operazioni, parametri da XSD inline, import relativi; compila automaticamente endpoint, versione, namespace, action e parametri.

## Estrazione dati

- **JSONPath** minimale: `$`, `$.a.b`, indici `$[0]`, wildcard `[*]`, `statusCode`, `headers.Nome`.
- **XPath 1.0** per risposte XML (es. `//*[local-name()='AddResult']/text()`), dispatch automatico.
- Suggerimenti automatici delle espressioni dall'ultima risposta (wizard link).

## Console e cronologia

- Console colorata (✓ successo, ✗ errore, ▶ header flusso, • nodi variabili).
- Cronologia persistita (`%APPDATA%/Rest Cu Me/.rcm-history`).
- Scheda cronologia: chip di stato, titolo · metodo · URL · timestamp, dettaglio Risposta/Richiesta, esplorazione JSON ad albero, **Copia JSON** / **Copia**, **Riesegui** (replay), **Pulisci cronologia**.
- Pulsante **Cronologia** nella barra del flusso per aprire il tool window.

## File

- Formato `.rcmflow` (JSON) identico al plugin IntelliJ originale, con migrazione automatica dei file legacy.
- Multi-scheda (più flussi aperti), Nuovo/Apri/Salva/Salva con nome.
