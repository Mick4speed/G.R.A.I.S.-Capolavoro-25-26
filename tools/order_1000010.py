#execute order 100 0010 on a specific file, input path (string), output state message (string)
#path
import os
import secrets

def execute(path):
    try:
        if not os.path.exists(path):
            return f"File does not exist at {path}"
        file_size = os.path.getsize(path)
        with open(path, 'wb') as file:
            for _ in range(6):
                file.seek(0)
                random_data = secrets.token_bytes(file_size)
                file.write(random_data)
                os.fsync(file.fileno())
        random_name = os.path.join(os.path.dirname(path), secrets.token_hex(8))
        os.rename(path, random_name)
        os.remove(random_name)
        print(f"Order 100 0010 executed successfully on {path}")
        return f"Order 100 0010 executed on {path}"
    except Exception as e:
        print(f"An error occurred while executing order 100 0010 on {path}: {str(e)}")
        return f"An error occurred: {str(e)}"