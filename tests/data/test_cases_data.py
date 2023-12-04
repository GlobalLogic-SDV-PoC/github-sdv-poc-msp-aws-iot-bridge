import pytest

temp_testdata_positive = [
    pytest.param("temp", 100, id="SDV-125 Positive int temp"),
    pytest.param("temp", 200, id="SDV-125 Max positive int temp"),
    pytest.param("temp", 0, id="SDV-125 Zero temp"),
    pytest.param("temp", -1, id="SDV-125 Negative int temp"),
    pytest.param("temp", -99, id="SDV-125 Min negative int temp"),
]

temp_testdata_negative = [
    pytest.param("temp", 99.99999, id="SDV-125 Positive double temp"),
    pytest.param("temp", -11.11111, id="SDV-125 Negative double temp"),
    pytest.param("temp", "NaN", id="SDV-125 NaN temp"),
]

ram_testdata_positive = [
    pytest.param("ram", 100, id="SDV-126 Positive int ram"),
    pytest.param("ram", 32644220, id="SDV-126 Max positive int ram"),
    pytest.param("ram", 0, id="SDV-126 Zero int ram"),
]

ram_testdata_negative = [
    pytest.param("ram", 32644221, id="SDV-126 Max + 1 positive int ram"),
    pytest.param("ram", -100, id="SDV-126 negative int ram"),
]
