#Read and print the text content of a file, the content is not returned but printed to the console, otherwise it would be returned twice, input path (string), output null (void)
#path
#REQUIREMENTS: PyPDF2, python-docx
import os
from pypdf import PdfReader
import docx 

def readPdf(path):
    reader = PdfReader(path)
    text = ""
    for page in reader.pages:
        for line in page.extract_text().splitlines():
            print(line)
        text += page.extract_text() + "\n"

def readWordDocument(path):
    document = docx.Document(path)
    text = ""
    for paragraph in document.paragraphs:
        if paragraph.text.strip():
            for line in paragraph.text.splitlines():
                print(line)
            text += paragraph.text + "\n"
file_types = {
    "pdf": readPdf,
    "docx": readWordDocument
}

def execute(path):
    if not os.path.isfile(path):
        print("Error: File does not exist.")
    try:
        extension = os.path.splitext(path)[1][1:].lower()
        if extension in file_types:
            file_types[extension](path)
        else:
            file =  open(path, 'r', encoding='utf-8')
            content = file.read()
            for line in content.splitlines():
                print(line)
    except Exception as e:
        print(f"Error: {str(e)}")