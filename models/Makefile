MODELS := $(wildcard models/source_models/*.ply)

models/models.c: $(MODELS) models/generate_model_source.lua models/generate_model_source.awk models/models.h
	luajit models/generate_model_source.lua $(MODELS) > models/models.c

models/models.h: $(MODELS) models/generate_model_source.lua models/generate_model_source.awk models/generate_model_header.awk
	awk -f models/generate_model_header.awk $(MODELS) > models/models.h