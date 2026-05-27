#Read the text content of a file, input path (string), output file content (string)
#path
#REQUIREMENTS: PyPDF2, python-docx
import os
from pypdf import PdfReader
import docx 

def readPdf(path):
    reader = PdfReader(path)
    text = ""
    for page in reader.pages:
        text += page.extract_text() + "\n"
    return text.strip()

def readWordDocument(path):
    document = docx.Document(path)
    text = ""
    for paragraph in document.paragraphs:
        if paragraph.text.strip():
            text += paragraph.text + "\n"
    return text

file_types = {
    "pdf": readPdf,
    "docx": readWordDocument
}

def execute(path):
    if not os.path.isfile(path):
        print("Error: File does not exist.")
        return "Error: File does not exist."
    try:
        extension = os.path.splitext(path)[1][1:].lower()
        if extension in file_types:
            content = file_types[extension](path)
            return content
        else:
            file =  open(path, 'r', encoding='utf-8')
            content = file.read()
            return content
    except Exception as e:
        print(f"Error: {str(e)}")
        return f"Error: {str(e)}"