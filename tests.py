import subprocess
import pathlib
import tempfile

import sys
import logging
import json


class ColoredFormatter(logging.Formatter):
    light_grey = "\x1b[90;20m"
    grey = "\x1b[38;20m"
    yellow = "\x1b[33;20m"
    red = "\x1b[31;20m"
    bold_red = "\x1b[31;1m"
    reset = "\x1b[0m"

    FORMATS: dict[int, str]
    FORMATTERS: dict[int, logging.Formatter]

    def __init__(self, format: str, style: str):
        self.FORMATS = {
            logging.DEBUG: self.light_grey + format + self.reset,
            logging.INFO: self.grey + format + self.reset,
            logging.WARNING: self.yellow + format + self.reset,
            logging.ERROR: self.red + format + self.reset,
            logging.CRITICAL: self.bold_red + format + self.reset,
        }
        self.FORMATTERS = {
            logging.DEBUG: logging.Formatter(self.FORMATS[logging.DEBUG],
                                             style=style),
            logging.INFO: logging.Formatter(self.FORMATS[logging.INFO],
                                            style=style),
            logging.WARNING: logging.Formatter(self.FORMATS[logging.WARNING],
                                               style=style),
            logging.ERROR: logging.Formatter(self.FORMATS[logging.ERROR],
                                             style=style),
            logging.CRITICAL:
                logging.Formatter(self.FORMATS[logging.CRITICAL], style=style),
        }

    def format(self, record: logging.LogRecord):
        return self.FORMATTERS[record.levelno].format(record)


def getLogger(name: str = None) -> logging.Logger:
    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)
    ch = logging.StreamHandler(sys.stdout)
    ch.setLevel(logging.DEBUG)
    formatter = ColoredFormatter(
        "{levelname:<6} ({name}): {message}", style='{')
    ch.setFormatter(formatter)
    logger.addHandler(ch)
    return logger


base_logger = getLogger("rbc-tests")

# build the code
result = subprocess.run(["./build.lua"])
if result.returncode != 0:
    base_logger.error("Failed to build the project, can not run tests.")
    exit(result.returncode)

tests_dir = pathlib.Path("./tests")


class Test:
    test_name: str
    input: str
    file: pathlib.Path

    def __init__(self, test_name: str, input: str, file: pathlib.Path):
        self.test_name = test_name
        self.input = input
        self.file = file

    def run_test(self) -> bool:
        logger = getLogger(self.test_name)

        ast_find_str = "--- ast ---\n"
        ast_start = self.input.find(ast_find_str)
        ir_find_str = "--- ir ---\n"
        ir_start = self.input.find(ir_find_str)
        run_find_str = "--- run ---\n"
        run_start = self.input.find(run_find_str)

        if ast_start == -1:
            logger.error(
                "Test %s(%s) does not contain a valid ast section",
                self.test_name,
                self.file)
            return False

        if ir_start == -1:
            logger.error(
                "Test %s(%s) does not contain a valid ir section",
                self.test_name,
                self.file)
            return False

        code = self.input[:ast_start]
        ast = self.input[len(ast_find_str)+ast_start:ir_start]
        ir = self.input[len(
            ir_find_str)+ir_start:run_start if run_start != -1 else len(self.input)]

        code = code.strip()
        ast = ast.strip()
        ir = ir.strip()

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = pathlib.Path(temp_dir)
            temp_code_file = temp_path / self.file.name

            with temp_code_file.open("w") as code_file:
                code_file.write(code)
                logger.debug("Written code section to %s", temp_code_file)

            logger.info("Running ast test section")
            print_ast_result = subprocess.run(
                ["./build/rbc", str(temp_code_file),
                 "--print=ast", "--no-emit"],
                capture_output=True)
            if print_ast_result.returncode != 0:
                logger.error(
                    "./build/rbc failed with %d, stdout: %s, stderr: %s",
                    print_ast_result.stdout, print_ast_result.stderr)
                return False
            stdout = str(print_ast_result.stdout.decode('utf8')).strip()
            if stdout != ast:
                logger.error("Expected \"%s\" to be \"%s\"", stdout, ast)
                return False

            logger.info("Running ir test section")
            print_ir_result = subprocess.run(
                ["./build/rbc", str(temp_code_file),
                 "--print=ir", "--no-emit"],
                capture_output=True)
            if print_ir_result.returncode != 0:
                logger.error(
                    "./build/rbc failed with %d, stdout: %s, stderr: %s",
                    print_ir_result.stdout, print_ir_result.stderr)
                return False
            stdout = str(print_ir_result.stdout.decode('utf8')).strip()
            if stdout != ir:
                logger.error("Expected:\n%s\n to be:\n%s", stdout, ir)
                return False

        if run_start != -1:
            run = self.input[len(run_find_str)+run_start:]
            loaded_json = json.loads(run)
            return_code = loaded_json["return_code"]
            temp_exe = temp_code_file.with_suffix("")
            compile = subprocess.run(
                ["./build/rbc", str(temp_code_file), "-o", str(temp_exe)])
            if compile.returncode != 0:
                logger.error("./build/rbc failed to compile test", )
                return False

            test = subprocess.run([str(temp_exe)])
            if test.returncode != return_code:
                logger.error(
                    "{} test failed, expected return code {}, got {}",
                    return_code, test.returncode)
                return False

        logger.info("Success")
        return True


failed = False

for file in tests_dir.iterdir():
    if file.is_file():
        test = Test(file.with_suffix("").name, file.read_text(), file)
        if not test.run_test():
            failed = True
