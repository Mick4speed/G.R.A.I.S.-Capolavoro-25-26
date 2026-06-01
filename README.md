## G.R.A.I.S. 

## Sommario 

[G.R.A.I.S.](#G.R.A.I.S.)
- [Sommario](#sommario)
- [Introduzione](#introduzione)
- [Struttura](#struttura)
  - [Brain](#brain)
  - [Rag Memory](#rag-memory)
  - [Python Runtime](#python-runtime)
  - [Interface](#interface)
- [Scaricare e Compilare G.R.A.I.S.](#scaricare-e-compilare-grais)
- [Riferimenti](#riferimenti)

## Introduzione 

G.R.A.I.S. (Generic Retrieval Agentic Intelligent System) è un agente IA chat-bot interamente eseguito sulla macchina dell’utente, che è in grado di compiere azioni in autonomia. 

È in grado di “imparare” informazioni da internet, salvandole in una memoria semantica, dove i dati vengono salvati non in ordine alfabetico o in ordine di inserimento, ma in base al significato dei dati. 

L’ho creato perché’ volevo avere un assistente in grado di aiutarmi durante il lavoro al computer. 

I vantaggi rispetto ad altri sistemi simili che adottano un modello in cui il modello IA è eseguito nel cloud, sono: 

- Maggiore privacy. 

- Impronta ambientale minore. 

- Più flessibilità nell’ampliare le funzionalità. 

- Più indipendenza da internet e da servizi terzi. 

Ci sono anche degli svantaggi riguardanti questa architettura: 

- Richiede un computer di medie/alte prestazioni. 

- Piu complesso da convertire per gli altri sistemi operativi. 

- Il modello IA è meno accurato perché è più compatto. 

## Struttura 

### Brain 

Brain è il cervello di GRAIS, qui viene gestito la LLM[1] e le chiamate ai tool. 

Quando l’utente chiede qualcosa a GRAIS, il prompt viene passato al modello LLM all’interno di un ciclo while, il ciclo si ferma se la LLM viene chiamata più di 5 volte, in questo caso la risposta viene considerata nulla perché il modello è andato in tilt. Il modello risponde in formato JSON[2] in cui viene specificato il tool da usare, il tool 0 è un tool speciale perché rompe il ciclo immediatamente e ritorna la risposta della LLM all’utente, negli altri casi dopo che i tool vengono eseguiti ritorniamo al modello il loro risultato come prompt. 

Per l’inferenza della LLM usiamo llama.cpp[3] , perché supporta i modelli gguf[4] , questi modelli sono delle versioni quantizzate e compresse in un singolo file di modelli normali, e quindi perfetti per l’uso nell’Edge AI[5] . 

Per analizzare i JSON usiamo  json.hpp[6] una libreria open-source, e facile da usare, e molto portabile tra i vari sistemi operativi perché composta da un solo file, chiamato appunto _“json.hpp”_ . 

### Rag Memory 

La memoria semantica di cui GRAIS è dotata, permette le 3 operazioni fondamentali: lettura, scrittura e rimozione. 

La memoria semantica funziona attraverso 3 componenti: un database vettoriale, un database relazionale, ed un modello embedder. 

Il modello embedder[7] converte una frase in un vettore multidimensionale (nel nostro caso 1024 dimensioni), dove la direzione in cui il vettore punta rappresenta il significato della frase, mentre la sua magnitudine (lunghezza) rappresenta quanto lungo era il testo. 

Il vettore insieme ad un ID viene salvato nel database vettoriale, mentre lo stesso ID e il testo corrispondente, insieme ad altri metadati, vengono salvati nel database relazionale. 

Come Database Vettoriale ho usato hnwslib[8] , che sfrutta l’algoritmo _“Approximate Nearest Neighbor”_ , invece come database relazionale ho usato sqlite[9] , mentre come modello di embedding ho usato bgem3[10] , per la sua leggerezza ed il suo supporto ad oltre 100 lingue. 

### Python Runtime 

Per eseguire codice python[11] all’interno di un’applicazione scritta in C++ ho usato le librerie ufficiali Python. 

Il sistema python ha 2 funzioni fondamentali: 

1. _executeString_ : Questa funzione prende in input uno script e lo esegue, e ritorna il testo intercettato dall’output di python. 

2. _executeTool_ : questa funzione prende in input l’ID di un tool python salvato nella sottocartella _tools_ . Si possono creare altri tool semplicemente aggiungendo un nuovo file python alla 

sottocartella prima di avviare GRAIS. 

I tool python devono rispettare questa struttura: 

- La prima riga deve essere un commento che abbia in ordine: descrizione, nomi e tipi dei parametri di input, nome tipo dell’output 

- La seconda riga deve essere un commento che contiene i nomi dei parametri di input distanziati da uno spazio 

- Il codice principale del tool deve stare nella funzione “ _execute”_ 

Esempio prime 2 righe tool python: 

_#Read the text content of a file, input path (string), output file content (string) #path_ 

### Interface 

Interface è un’interfaccia che gestisce la comunicazione tra il Brain e la Memoria RAG, e racchiude in una singola funzione l’operazione di web scraping[12] . 

## Scaricare e Compilare G.R.A.I.S. 

Prima di poter scaricare e compilare GRAIS bisogna aver installati tre strumenti fondamentali: 

- Python[13] , per permettere a GRAIS di eseguire codice senza doverlo compilare 

- CUDA[14] , per eseguire il modello sfruttando la potenza delle GPU Nvidia 

- Visual Studio[15] ,  insieme al componente aggiuntivo _“Sviluppo di applicazioni C++”_ , per poter modificare e compilare il codice sorgente 

Poi, scarichiamo il progetto[16] , e creiamo una sottocartella _models_ dove bisogna scaricare la LLM[17] e l’Embedder e li rinominiamo rispettivamente _“LLM.gguf”_ e _“Embedder.gguf”_ . 

Poi bisogna scarica llama.cpp[18] , il motore che si occuperà l’inferenza dei modelli IA presenti dentro GRAIS (in parole semplice eseguirà i modelli), dopo averla scaricata bisogna compilarla[19] usando i flag 

specifici per l’accelerazione GPU CUDA. Dopo bisogna aprire un finestra _Developer PowerShell for Visual_ Studio nella cartella _build_ che è stata creata ed eseguire questo comando: 

_“cmake --install . --config Release --prefix "Percorso ad una qualsiasi cartella"”_ 

Questo comando creera nella cartella indicata 3 sottocartelle _bin include_ e _lib_ , le ultime due andranno copiate dentro la cartella principale del progetto, mentre dalla cartella bin vanno presi i DLL e messi nella cartella dell’eseguibile, che si troverà (dopo aver provato a compilare ed eseguire GRAIS per la prima volta) in _“Cartella Progetto/x64/Release”_ . 

Ora bisogna che il percorso di Python impostati nel linker e nel compilatore della configurazione del progetto siano li stessi di quelli presenti nel proprio sistema, essi cambiano ad ogni versione, per esempio per la versione di python 3.13 il percorso sarà: 

## _“$(LOCALAPPDATA)\Programs\Python\Python313\libs”_ 

## _“$(LOCALAPPDATA)\Programs\Python\Python313\include”_ 

Ora aprendo il progetto (.vcxproj) o la soluzione (.sln) con visual studio, possiamo compilare ed eseguire GRAIS 

## Riferimenti 

- (1) Lee, A. _What Are Large Language Models Used For?_ . NVIDIA Blog. 

   - https://blogs.nvidia.com/blog/what-are-large-language-models-used-for/ (accessed 2026-06-01). 

- (2) _Working with JSON - Learn web development | MDN_ . MDN Web Docs. https://developer.mozilla.org/en-US/docs/Learn_web_development/Core/Scripting/JSON (accessed 2026-06-01). 

- (3) Ggml-Org/Llama.Cpp, 2026. https://github.com/ggml-org/llama.cpp (accessed 2026-06-01). 

- (4) _ggml/docs/gguf.md at master · ggml-org/ggml_ . GitHub. https://github.com/ggmlorg/ggml/blob/master/docs/gguf.md (accessed 2026-06-01). 

- (5) Smalley, S. S., Ian. _Che cos’è l’edge AI? | IBM_ . https://www.ibm.com/it-it/think/topics/edge-ai (accessed 2026-06-01). 

- (6) Lohmann, N. _JSON for Modern C++_ . https://github.com/nlohmann (accessed 2026-06-01). 

- (7) _What is Text Embedding?_ . GeeksforGeeks. https://www.geeksforgeeks.org/nlp/what-is-textembedding/ (accessed 2026-06-01). 

- (8) _Hnswlib – Fast & Accurate Nearest Neighbor Search Library_ . https://hnswlib.com/ (accessed 2026-06-01). 

- (9) _SQLite Home Page_ . https://sqlite.org/ (accessed 2026-06-01). 

- (10) _gpustack/bge-m3-GGUF · Hugging Face_ . https://huggingface.co/gpustack/bge-m3-GGUF (accessed 2026-06-01). 

- (11) _The Python Tutorial_ . Python documentation. https://docs.python.org/3/tutorial/index.html (accessed 2026-06-01). 

- (12) _What is Web Scraping and How to Use It?_ . GeeksforGeeks. 

   - https://www.geeksforgeeks.org/blogs/what-is-web-scraping-and-how-to-use-it/ (accessed 202606-01). 

- (13) _Download Python_ . Python.org. https://www.python.org/downloads/ (accessed 2026-06-01). 

- (14) _CUDA Toolkit 12.1 Downloads_ . NVIDIA Developer. https://developer.nvidia.com/cuda-downloads (accessed 2026-06-01). 

- (15) _Download di Visual Studio e VS Code per Windows, Mac e Linux_ . Visual Studio. https://visualstudio.microsoft.com/it/downloads/ (accessed 2026-06-01). 

- (16) Michael Joseph Junior Mancuso. Mick4speed/G.R.A.I.S.-Capolavoro-25-26, 2026. https://github.com/Mick4speed/G.R.A.I.S.-Capolavoro-25-26 (accessed 2026-06-01). 

- (17) _NousResearch/Hermes-3-Llama-3.1-8B-GGUF · Hugging Face_ . 

   - https://huggingface.co/NousResearch/Hermes-3-Llama-3.1-8B-GGUF (accessed 2026-06-01). 

- (18) _llama.cpp/docs/build.md at master · ggml-org/llama.cpp_ . GitHub. https://github.com/ggmlorg/llama.cpp/blob/master/docs/build.md (accessed 2026-06-01). 

- (19) _llama.cpp/docs/build.md at master · ggml-org/llama.cpp_ . GitHub. https://github.com/ggmlorg/llama.cpp/blob/master/docs/build.md#cuda (accessed 2026-06-01). 

