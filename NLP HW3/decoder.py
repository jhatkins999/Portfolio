from conll_reader import DependencyStructure, DependencyEdge, conll_reader
from collections import defaultdict
import copy
import sys

import numpy as np
import keras

from extract_training_data import FeatureExtractor, State

class Parser(object): 

    def __init__(self, extractor, modelfile):
        self.model = keras.models.load_model(modelfile)
        self.extractor = extractor
        
        # The following dictionary from indices to output actions will be useful
        self.output_labels = dict([(index, action) for (action, index) in extractor.output_labels.items()])

    def parse_sentence(self, words, pos):
        state = State(range(1,len(words)))
        state.stack.append(0)    
        while state.buffer: 
            rep = self.extractor.get_input_representation(words, pos, state)
            rep = np.array([rep])
            vector = self.model.predict([rep])[0]   
            vector_sort = np.sort(vector)[::-1]
            for prob in vector_sort:
                label = self.output_labels[list(vector).index(prob)]
                if not state.stack and label[0] != 'shift':
                    continue
                elif len(state.buffer) < 2 and label[0] == 'shift' and state.stack:
                    continue
                elif label[1] is None and label[0] == 'left_arc':
                    continue
                else:
                    if label[0] == 'shift':
                        state.shift()
                        break
                    elif label[0] == 'left_arc':
                        state.left_arc(label[1])
                        break
                    elif label[0] == 'right_arc':
                        state.right_arc(label[1])
                        break
                    else:
                        print("failure")
                    
        result = DependencyStructure()
        for p,c,r in state.deps: 
            result.add_deprel(DependencyEdge(c,words[c],pos[c],p, r))
        return result 
        

if __name__ == "__main__":

    WORD_VOCAB_FILE = 'data/words.vocab'
    POS_VOCAB_FILE = 'data/pos.vocab'

    try:
        word_vocab_f = open(WORD_VOCAB_FILE,'r')
        pos_vocab_f = open(POS_VOCAB_FILE,'r') 
    except FileNotFoundError:
        print("Could not find vocabulary files {} and {}".format(WORD_VOCAB_FILE, POS_VOCAB_FILE))
        sys.exit(1) 

    extractor = FeatureExtractor(word_vocab_f, pos_vocab_f)
    parser = Parser(extractor, sys.argv[1])

    with open(sys.argv[2],'r') as in_file: 
        for dtree in conll_reader(in_file):
            words = dtree.words()
            pos = dtree.pos()
            deps = parser.parse_sentence(words, pos)
            print(deps.print_conll())
            print()
        
