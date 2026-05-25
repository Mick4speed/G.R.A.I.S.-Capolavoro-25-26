#create a simple text-only PDF file with the specified content, input path(string) content (string), output state message (string)
#path content
import reportlab
from reportlab.lib.pagesizes import letter
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT

def execute(path, content):
    # 1. Configura il documento con i margini (50 punti per lato, ~1.7 cm)
    # Questo definisce l'area "sicura" in cui il testo si stringe senza essere tagliato
    doc = SimpleDocTemplate(
        path,
        pagesize=letter,
        rightMargin=50,
        leftMargin=50,
        topMargin=50,
        bottomMargin=50
    )
    
    # 2. Configura lo stile "Stile Blocco Note / TXT"
    # Usiamo il font Courier monospazio per mantenere l'effetto file .txt
    stile_txt = ParagraphStyle(
        name='StileTxt',
        fontName='Courier',
        fontSize=10,
        leading=12,          # L'interlinea (spazio tra le righe)
        alignment=TA_LEFT,   # Allineato a sinistra
    )
    
    # 3. Prepariamo la lista di elementi (story) da inserire nel PDF
    story = []
    
    # Dividiamo il testo in base agli andata a capo (\n) forniti in input
    righe = content.split('\n')
    
    for riga in righe:
        if riga.strip() == "":
            # Se la riga è vuota, aggiungiamo uno spazio vuoto verticale pari all'interlinea
            story.append(Spacer(1, 12))
        else:
            # Sostituiamo gli spazi multipli con spazi non-interrompibili (&nbsp;)
            # Questo preserva le indentazioni o le spaziature fisse del testo originale
            riga_formattata = riga.replace(" ", "&nbsp;")
            
            # Creiamo il paragrafo. Se la riga supera il margine destro, 
            # andrà a capo automaticamente in modo sicuro.
            p = Paragraph(riga_formattata, stile_txt)
            story.append(p)
            
    # 4. Generiamo il PDF (la gestione delle pagine multiple è completamente automatica)
    doc.build(story)
    
    # Restituiamo il messaggio nello stesso formato richiesto
    output_message = f"🎉 PDF created successfully: {path}"
    print(output_message)
    return output_message