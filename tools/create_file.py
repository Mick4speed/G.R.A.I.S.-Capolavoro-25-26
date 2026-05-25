#Write a file with in the specified path and with the content provided, input path (string) content (string), output state message (string)
#path content 
def execute(path, content):
    try:
        print("Writing file at:", path)
        with open(path, 'w') as file:
            file.write(content)
        print("File written successfully at:", path)
        return f"File written successfully at {path}"
    except Exception as e:
        print("An error occurred while writing file at:", path, "Error:", str(e))
        return f"An error occurred: {str(e)}"