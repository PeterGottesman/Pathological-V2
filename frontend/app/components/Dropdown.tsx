import { useEffect, useRef, useState } from 'react';

type Option = {
    label: string;
    value: string;
}

interface DropdownProps {
    options: Option[];
    placeholder?: string;
    selected: Option | null;
    onSelectedChanges: (option: Option) => void;
}

const Dropdown: DropdownProps = ({options, placeholder = 'Select scene', selected, onSelectedChanges}) => {
    const [isOpen, setIsOpen] = useState<boolean>(false);
    const ref = useRef<HTMLDivElement | null>(null);
    
    //make dropdown close when user clicks outside of it
    useEffect( () => {
        const onBodyClick = (event:MouseEvent) => {
            if(ref.current && ref.current.contains(event.target as Node)) {
                return;
            }

            setIsOpen(false);
        }

        document.body.addEventListener('click',onBodyClick);

        return () => {
            document.body.removeEventListener('click',onBodyClick);
        }
    });
    const toggleDropdown = () => {
        setIsOpen(prevState => !prevState)
    };

    return (
        <div ref={ref}>
            <div className="w-full">
                <div className="relative w-full">
                    <button 
                        onClick={toggleDropdown}
                        className="mt-1 w-full cursor-pointer rounded-lg border border-red-600 px-3 py-2 font-semibold transition-all hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)]">
                        <div className="flex justify-between w-full">
                            {
                                selected ?
                                    <div className="flex gap-x-2">
                                        <span className="block text-sm text-red-300 truncate">{selected.label}</span>
                                    </div>
                                    : <span className="block text-sm text-red-700 truncate">{placeholder}</span>
                            }
                        </div>
                    </button>
                    {
                        isOpen &&
                        <ul className="absolute z-10 mt-1 w-full bg-black shadow-lg max-w-[220px] rounded-md py-1 border border red-600 ring-red-600 overflow-auto focus:outline-none text-sm">
                            {options.map(option => (
                                <li key={option.value}
                                    onClick={() => onSelectedChanges(option)} 
                                    className={`${selected?.value === option.value && 'bg-slate-900'} hover:bg-slate-900 transition-all text-red-500 hover:text-red-200 hover:drop-shadow-[0_0_10px_rgba(220,38,38,0.9)] flex justify-between cursor-pointer text-gray-900 select-none relative py-2 px-3 `}
                                >
                                    <div className="flex items-center gap-x-2">
                                        <span className="block text-sm truncate">{option.label}</span>
                                    </div>
                                </li>
                            ))
                            }
                        </ul>
                    }
                </div>
            </div>
        </div>
    )
}

export default Dropdown;